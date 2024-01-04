#include <fcntl.h>
#include "com_file.h"
#include "com_log.h"
#include "comex_smb.h"
#include "smb2/libsmb2.h"
#include "smb2/smb2-errors.h"
#include "smb2/smb2.h"


CPPSmb::CPPSmb()
{
    dir_local = "." PATH_DELIM_STR;
}

CPPSmb::~CPPSmb()
{
    disconnect();
}

CPPSmb& CPPSmb::setShareUrlPath(const char* url)
{
    if(url != NULL)
    {
        this->url = url;
    }
    return *this;
}

CPPSmb& CPPSmb::setHost(const char* host)
{
    if(host != NULL)
    {
        this->host = host;
    }
    return *this;
}

CPPSmb& CPPSmb::setShareName(const char* share)
{
    if(share != NULL)
    {
        this->share_name = share;
    }
    return *this;
}

CPPSmb& CPPSmb::setDomain(const char* domain)
{
    if(domain != NULL)
    {
        this->domain = domain;
    }
    return *this;
}

CPPSmb& CPPSmb::setUsername(const char* user)
{
    if(user != NULL)
    {
        this->username = user;
    }
    return *this;
}

CPPSmb& CPPSmb::setPassword(const char* pwd)
{
    if(pwd != NULL)
    {
        this->password = pwd;
    }
    return *this;
}

CPPSmb& CPPSmb::setWorkstation(const char* workstation)
{
    if(workstation != NULL)
    {
        this->workstation = workstation;
    }
    return *this;
}

CPPSmb& CPPSmb::setLocalDir(const char* dir)
{
    if(dir != NULL)
    {
        this->dir_local = dir;
        if(com_string_end_with(dir_local.c_str(), PATH_DELIM_STR) == false)
        {
            dir_local += PATH_DELIM_STR;
        }
    }
    return *this;
}

CPPSmb& CPPSmb::setRemoteDir(const char* dir)
{
    if(dir != NULL)
    {
        this->dir_remote = dir;
        if(com_string_end_with(dir_remote.c_str(), PATH_DELIM_STR) == false)
        {
            dir_remote += PATH_DELIM_STR;
        }
    }
    return *this;
}

bool CPPSmb::autoReconnect()
{
    if(smb_ctx != NULL && smb2_echo((struct smb2_context*)smb_ctx) == SMB2_STATUS_SUCCESS)
    {
        return true;
    }

    disconnect();
    smb_ctx = smb2_init_context();
    if(smb_ctx == NULL)
    {
        LOG_E("init smb ctx failed");
        return false;
    }

    if(url.empty() == false)
    {
        struct smb2_url* result = smb2_parse_url((struct smb2_context*)smb_ctx, url.c_str());
        if(result != NULL)
        {
            if(result->server != NULL)
            {
                host = result->server;
            }
            if(result->share != NULL)
            {
                share_name = result->share;
            }
            if(result->user != NULL)
            {
                username = result->user;
            }
            if(result->domain != NULL)
            {
                domain = result->domain;
            }
            setRemoteDir(result->path);
            smb2_destroy_url(result);
        }
        if(url.find("?") == std::string::npos)//url中不带参数则不必重复调用smb2_parse_url
        {
            url.clear();
        }
    }

    if(username.empty() == false)
    {
        smb2_set_user((struct smb2_context*)smb_ctx, username.c_str());
    }

    if(password.empty() == false)
    {
        smb2_set_password((struct smb2_context*)smb_ctx, password.c_str());
    }

    if(domain.empty() == false)
    {
        smb2_set_domain((struct smb2_context*)smb_ctx, domain.c_str());
    }

    if(workstation.empty() == false)
    {
        smb2_set_workstation((struct smb2_context*)smb_ctx, workstation.c_str());
    }
    smb2_set_security_mode((struct smb2_context*)smb_ctx, SMB2_NEGOTIATE_SIGNING_ENABLED);
    int ret = smb2_connect_share((struct smb2_context*)smb_ctx, host.c_str(), share_name.c_str(), username.c_str());
    if(ret != 0)
    {
        LOG_E("connection failed,ret=%d:%s", ret, smb2_get_error((struct smb2_context*)smb_ctx));
    }

    return (ret == 0);
}

void CPPSmb::disconnect()
{
    if(smb_ctx != NULL)
    {
        smb2_disconnect_share((struct smb2_context*)smb_ctx);
        smb2_destroy_context((struct smb2_context*)smb_ctx);
        smb_ctx = NULL;
    }
}

bool CPPSmb::put(const char* file_path_local)
{
    std::string file_path_remote = dir_remote + com_path_name(file_path_local);
    return putAs(file_path_local, file_path_remote.c_str());
}

bool CPPSmb::putAs(const char* file_path_local, const char* file_path_remote)
{
    if(file_path_local == NULL || file_path_remote == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }

    if(autoReconnect() == false)
    {
        return false;
    }

    FILE* f_local = com_file_open(file_path_local, "rb");
    if(f_local == NULL)
    {
        LOG_E("failed to open local file:%s", file_path_local);
        return false;
    }

    if(this->mkdir(com_path_dir(file_path_remote).c_str()) == false)
    {
        com_file_close(f_local);
        LOG_E("failed to create dir");
        return false;
    }
    struct smb2fh* f_remote = smb2_open((struct smb2_context*)smb_ctx, file_path_remote, O_WRONLY | O_CREAT);
    if(f_remote == NULL)
    {
        com_file_close(f_local);
        LOG_E("failed to open remote file:%s", file_path_remote);
        return false;
    }

    uint8 buf[1024];
    int64 read_size = 0 ;
    int64 write_size = 0;
    bool write_succeed = true;
    while((read_size = com_file_read(f_local, buf, sizeof(buf))) > 0)
    {
        int ret = smb2_write((struct smb2_context*)smb_ctx, f_remote, buf, (uint32)read_size);
        if(ret < 0)
        {
            write_succeed = false;
            break;
        }
        write_size += ret;
    }
    com_file_close(f_local);
    if(write_succeed)
    {
        smb2_fsync((struct smb2_context*)smb_ctx, f_remote);
    }
    smb2_close((struct smb2_context*)smb_ctx, f_remote);
    return (write_size >= 0);
}

bool CPPSmb::get(const char* file_path_remote)
{
    if(file_path_remote == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }

    std::vector<std::string> vals = com_string_split(file_path_remote, PATH_DELIM_STR);
    if(vals.size() == 0)
    {
        LOG_E("split failed");
        return false;
    }

    std::string file_path_local = dir_local + vals[vals.size() - 1];
    return getAs(file_path_remote, file_path_local.c_str());
}

bool CPPSmb::getAs(const char* file_path_remote, const char* file_path_local)
{
    if(file_path_remote == NULL || file_path_local == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }

    com_dir_create(com_path_dir(file_path_local).c_str());
    FILE* f_local = com_file_open(file_path_local, "wb+");
    if(f_local == NULL)
    {
        LOG_E("failed to open local file:%s", file_path_local);
        return false;
    }

    if(autoReconnect() == false)
    {
        com_file_close(f_local);
        return false;
    }

    struct smb2fh* f_remote = smb2_open((struct smb2_context*)smb_ctx, file_path_remote, O_RDONLY);
    if(f_remote == NULL)
    {
        com_file_close(f_local);
        LOG_E("failed to open remote file:%s", file_path_remote);
        return false;
    }

    uint8 buf[1024];
    int read_size = 0 ;
    int64 write_size = 0;
    while((read_size = smb2_read((struct smb2_context*)smb_ctx, f_remote, buf, sizeof(buf))) > 0)
    {
        write_size += com_file_write(f_local, buf, read_size);
    }
    if(read_size < 0)
    {
        LOG_E("read error, ret=%d:%s", read_size, smb2_get_error((struct smb2_context*)smb_ctx));
    }
    com_file_flush(f_local);
    com_file_close(f_local);
    smb2_close((struct smb2_context*)smb_ctx, f_remote);
    return (write_size > 0);
}

CPPBytes CPPSmb::getBytes(const char* file_path_remote)
{
    if(file_path_remote == NULL)
    {
        LOG_E("arg incorrect");
        return CPPBytes();
    }

    if(autoReconnect() == false)
    {
        return CPPBytes();
    }

    struct smb2fh* f_remote = smb2_open((struct smb2_context*)smb_ctx, file_path_remote, O_RDONLY);
    if(f_remote == NULL)
    {
        LOG_E("failed to open remote file:%s", file_path_remote);
        return CPPBytes();
    }

    CPPBytes bytes;
    uint8 buf[1024];
    int read_size = 0 ;
    while((read_size = smb2_read((struct smb2_context*)smb_ctx, f_remote, buf, sizeof(buf))) > 0)
    {
        bytes.append(buf, read_size);
    }
    smb2_close((struct smb2_context*)smb_ctx, f_remote);
    return bytes;
}

std::map<std::string, int> CPPSmb::ls(const char* dir_path, bool full_path)
{
    std::map<std::string, int> list;
    if(dir_path == NULL)
    {
        LOG_E("arg incorrect");
        return list;
    }

    if(autoReconnect() == false)
    {
        return list;
    }

    struct smb2dir* fp_dir = smb2_opendir((struct smb2_context*)smb_ctx, dir_path);
    if(fp_dir == NULL)
    {
        LOG_E("failed to open remote dir:%s", dir_path);
        return list;
    }

    std::string dir;
    if(full_path)
    {
        dir = dir_path;
        if(com_string_end_with(dir_path, PATH_DELIM_STR) == false)
        {
            dir += PATH_DELIM_STR;
        }
    }
    struct smb2dirent* ent = NULL;
    while((ent = smb2_readdir((struct smb2_context*)smb_ctx, fp_dir)) != NULL)
    {
        if(ent->name == NULL || com_string_equal(ent->name, ".") || com_string_equal(ent->name, ".."))
        {
            continue;
        }
        switch(ent->st.smb2_type)
        {
            case SMB2_TYPE_LINK:
                list[dir + ent->name] = FILE_TYPE_LINK;
                break;
            case SMB2_TYPE_FILE:
                list[dir + ent->name] = FILE_TYPE_FILE;
                break;
            case SMB2_TYPE_DIRECTORY:
                list[dir + ent->name] = FILE_TYPE_DIR;
                break;
            default:
                list[dir + ent->name] = FILE_TYPE_UNKNOWN;
                break;
        }
    }
    smb2_closedir((struct smb2_context*)smb_ctx, fp_dir);
    return list;
}

bool CPPSmb::rename(const char* file_path, const char* file_path_new)
{
    if(file_path == NULL || file_path_new == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }

    if(autoReconnect() == false)
    {
        return false;
    }

    return smb2_rename((struct smb2_context*)smb_ctx, file_path, file_path_new) == 0;
}

bool CPPSmb::clean(const char* file_path)
{
    if(file_path == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }

    if(autoReconnect() == false)
    {
        return false;
    }

    return smb2_truncate((struct smb2_context*)smb_ctx, file_path, 0) == 0;
}

bool CPPSmb::remove(const char* file_path)
{
    if(file_path == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }

    if(autoReconnect() == false)
    {
        return false;
    }

    return smb2_unlink((struct smb2_context*)smb_ctx, file_path) == 0;
}

bool CPPSmb::mkdir(const char* dir_path)
{
    if(dir_path == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }

    if(autoReconnect() == false)
    {
        return false;
    }

    std::vector<std::string> paths = com_string_split(dir_path, PATH_DELIM_STR);
    if(paths.empty())
    {
        LOG_E("path incorrect");
        return false;
    }

    std::string path;
    for(size_t i = 0; i < paths.size(); i++)
    {
        path.append(paths[i]);
        if(path.empty())
        {
            path += PATH_DELIM_STR;
            continue;
        }
        if(path == "." || path == "..")
        {
            path += PATH_DELIM_STR;
            continue;
        }
        int file_type = getFileType(path.c_str());
        if(file_type != FILE_TYPE_NOT_EXIST)
        {
            if(file_type != FILE_TYPE_DIR)
            {
                LOG_E("file %s already exist", path.c_str());
                return false;
            }
            path += PATH_DELIM_STR;
            continue;
        }
        if(smb2_mkdir((struct smb2_context*)smb_ctx, path.c_str()) != 0)
        {
            LOG_E("make dir %s failed,path=%s", path.c_str(), dir_path);
            return false;
        }
        path += PATH_DELIM_STR;
    }

    return true;
}

bool CPPSmb::rmdir(const char* dir_path)
{
    if(dir_path == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }

    if(autoReconnect() == false)
    {
        return false;
    }

    struct smb2dir* fp_dir = smb2_opendir((struct smb2_context*)smb_ctx, dir_path);
    if(fp_dir == NULL)
    {
        return false;
    }

    struct smb2dirent* ent = NULL;
    while((ent = smb2_readdir((struct smb2_context*)smb_ctx, fp_dir)) != NULL)
    {
        if(strcmp(ent->name, ".") == 0 || strcmp(ent->name, "..") == 0)
        {
            continue;
        }
        std::string path = dir_path;
        path.append(PATH_DELIM_STR);
        path.append(ent->name);

        if(ent->st.smb2_type == SMB2_TYPE_DIRECTORY)
        {
            this->rmdir(path.c_str());
        }
        else
        {
            smb2_unlink((struct smb2_context*)smb_ctx, path.c_str());
        }
    }

    smb2_closedir((struct smb2_context*)smb_ctx, fp_dir);
    smb2_rmdir((struct smb2_context*)smb_ctx, dir_path);
    return true;
}

int CPPSmb::getFileType(const char* file_path_remote)
{
    if(file_path_remote == NULL)
    {
        return FILE_TYPE_NOT_EXIST;
    }

    if(autoReconnect() == false)
    {
        return FILE_TYPE_UNKNOWN;
    }

    struct smb2_stat_64 st;
    memset(&st, 0, sizeof(st));
    if(smb2_stat((struct smb2_context*)smb_ctx, file_path_remote, &st) != 0)
    {
        return FILE_TYPE_NOT_EXIST;
    }

    if(st.smb2_type == SMB2_TYPE_FILE)
    {
        return FILE_TYPE_FILE;
    }

    if(st.smb2_type == SMB2_TYPE_DIRECTORY)
    {
        return FILE_TYPE_DIR;
    }

    if(st.smb2_type == SMB2_TYPE_LINK)
    {
        return FILE_TYPE_LINK;
    }

    return FILE_TYPE_UNKNOWN;
}

int64 CPPSmb::getFileSize(const char* file_path_remote)
{
    if(file_path_remote == NULL)
    {
        return -1;
    }

    if(autoReconnect() == false)
    {
        return -1;
    }

    struct smb2_stat_64 st;
    memset(&st, 0, sizeof(st));
    if(smb2_stat((struct smb2_context*)smb_ctx, file_path_remote, &st) != 0)
    {
        return -1;
    }

    return (int64)st.smb2_size;
}

uint32 CPPSmb::getFileAccessTime(const char* file_path_remote)
{
    if(file_path_remote == NULL)
    {
        return 0;
    }

    if(autoReconnect() == false)
    {
        return 00;
    }

    struct smb2_stat_64 st;
    memset(&st, 0, sizeof(st));
    if(smb2_stat((struct smb2_context*)smb_ctx, file_path_remote, &st) != 0)
    {
        return 0;
    }

    return (uint32)st.smb2_atime;
}

uint32 CPPSmb::getFileModifyTime(const char* file_path_remote)
{
    if(file_path_remote == NULL)
    {
        return 0;
    }

    if(autoReconnect() == false)
    {
        return 0;
    }

    struct smb2_stat_64 st;
    memset(&st, 0, sizeof(st));
    if(smb2_stat((struct smb2_context*)smb_ctx, file_path_remote, &st) != 0)
    {
        return 0;
    }

    return (uint32)st.smb2_mtime;
}

uint32 CPPSmb::getFileChangeTime(const char* file_path_remote)
{
    if(file_path_remote == NULL)
    {
        return 0;
    }

    if(autoReconnect() == false)
    {
        return 0;
    }

    struct smb2_stat_64 st;
    memset(&st, 0, sizeof(st));
    if(smb2_stat((struct smb2_context*)smb_ctx, file_path_remote, &st) != 0)
    {
        return 0;
    }
    return (uint32)st.smb2_ctime;
}

