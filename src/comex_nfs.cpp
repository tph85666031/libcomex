#include <fcntl.h>
#include "com_file.h"
#include "com_log.h"
#include "comex_nfs.h"
#include "nfsc/libnfs.h"
#include "nfsc/libnfs-raw-nfs.h"

ComexNfs::ComexNfs()
{
    dir_local = "." PATH_DELIM_STR;
    dir_remote = "/";
}

ComexNfs::~ComexNfs()
{
    disconnect();
}
ComexNfs& ComexNfs::setShareUrlPath(const char* url)
{
    if(url != NULL)
    {
        this->url = url;
    }
    return *this;
}
ComexNfs& ComexNfs::setHost(const char* host)
{
    if(host != NULL)
    {
        this->host = host;
    }
    return *this;
}

ComexNfs& ComexNfs::setShareName(const char* share_name)
{
    if(share_name != NULL)
    {
        this->share_name = share_name;
    }
    return *this;
}

ComexNfs& ComexNfs::setUID(int uid)
{
    if(uid >= 0)
    {
        this->uid = uid;
    }
    return *this;
}

ComexNfs& ComexNfs::setGID(int gid)
{
    if(gid >= 0)
    {
        this->gid = gid;
    }
    return *this;
}

ComexNfs& ComexNfs::setVersion(int version)
{
    this->version = version;
    return *this;
}

ComexNfs& ComexNfs::seReconnect(bool reconnect)
{
    this->reconnect = reconnect;
    return *this;
}

ComexNfs& ComexNfs::setLocalDir(const char* dir)
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

ComexNfs& ComexNfs::setRemoteDir(const char* dir)
{
    if(dir != NULL)
    {
        this->dir_remote = dir;
        if(com_string_end_with(dir_remote.c_str(), "/") == false)
        {
            dir_remote += "/";
        }
    }
    return *this;
}

bool ComexNfs::autoReconnect()
{
    if(nfs_ctx != NULL && nfs_is_connected((struct nfs_context*)nfs_ctx))
    {
        return true;
    }
    disconnect();
    nfs_ctx = nfs_init_context();
    if(nfs_ctx == NULL)
    {
        LOG_E("init nfs ctx failed");
        return false;
    }

    nfs_set_uid((struct nfs_context*)nfs_ctx, uid);
    nfs_set_gid((struct nfs_context*)nfs_ctx, gid);
    nfs_set_autoreconnect((struct nfs_context*)nfs_ctx, reconnect ? -1 : 0);
    nfs_set_version((struct nfs_context*)nfs_ctx, version);

    if(url.empty() == false)
    {
        struct nfs_url* result = nfs_parse_url_dir((struct nfs_context*)nfs_ctx, url.c_str());
        if(result != NULL)
        {
            if(result->server != NULL)
            {
                host = result->server;
            }
            if(result->path != NULL)
            {
                share_name = result->path;
            }
            nfs_destroy_url(result);
        }
        if(url.find("?") == std::string::npos)//url中不带参数则不必重复调用nfs_parse_url_dir
        {
            url.clear();
        }
    }

    int ret = nfs_mount((struct nfs_context*)nfs_ctx, host.c_str(), share_name.c_str());
    if(ret == 0)
    {
        nfs_connected = true;
    }
    else
    {
        nfs_connected = false;
        LOG_E("connection failed,ret=%d:%s", ret, nfs_get_error((struct nfs_context*)nfs_ctx));
    }

    return nfs_connected;
}

void ComexNfs::disconnect()
{
    if(nfs_ctx != NULL)
    {
        nfs_umount((struct nfs_context*)nfs_ctx);
        nfs_destroy_context((struct nfs_context*)nfs_ctx);
        nfs_ctx = NULL;
    }
}

bool ComexNfs::put(const char* file_path_local)
{
    std::string file_path_remote = dir_remote + com_path_name(file_path_local);
    return putAs(file_path_local, file_path_remote.c_str());
}

bool ComexNfs::putAs(const char* file_path_local, const char* file_path_remote)
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
        LOG_E("failed to make dir:%s", com_path_dir(file_path_remote).c_str());
        return false;
    }

    struct nfsfh* f_remote = NULL;
    int ret = nfs_open((struct nfs_context*)nfs_ctx, PATH_FROM_DOS(file_path_remote).c_str(), O_WRONLY | O_CREAT, &f_remote);
    if(ret != 0)
    {
        com_file_close(f_local);
        LOG_E("failed to open remote file:%s", file_path_remote);
        return false;
    }

    uint8 buf[1024];
    int64 read_size = 0 ;
    int64 write_size = 0;
    while((read_size = com_file_read(f_local, buf, sizeof(buf))) > 0)
    {
        write_size += nfs_write((struct nfs_context*)nfs_ctx, f_remote, read_size, buf);
    }
    com_file_close(f_local);
    nfs_fsync((struct nfs_context*)nfs_ctx, f_remote);
    nfs_close((struct nfs_context*)nfs_ctx, f_remote);
    return (write_size >= 0);
}

bool ComexNfs::get(const char* file_path_remote)
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

bool ComexNfs::getAs(const char* file_path_remote, const char* file_path_local)
{
    if(file_path_local == NULL || file_path_remote == NULL)
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

    struct nfsfh* f_remote = NULL;
    int ret = nfs_open((struct nfs_context*)nfs_ctx, PATH_FROM_DOS(file_path_remote).c_str(), O_RDONLY, &f_remote);
    if(ret != 0)
    {
        com_file_close(f_local);
        LOG_E("failed to open remote file:%s", PATH_FROM_DOS(file_path_remote).c_str());
        return false;
    }

    uint8 buf[1024];
    int read_size = 0 ;
    int64 write_size = 0;
    while((read_size = nfs_read((struct nfs_context*)nfs_ctx, f_remote, sizeof(buf), buf)) > 0)
    {
        write_size += com_file_write(f_local, buf, read_size);
    }
    if(read_size < 0)
    {
        LOG_E("read error, ret=%d:%s", read_size, nfs_get_error((struct nfs_context*)nfs_ctx));
    }
    com_file_flush(f_local);
    com_file_close(f_local);
    nfs_close((struct nfs_context*)nfs_ctx, f_remote);
    return (write_size > 0);
}

ComBytes ComexNfs::getBytes(const char* file_path_remote)
{
    if(file_path_remote == NULL)
    {
        LOG_E("arg incorrect");
        return ComBytes();
    }

    if(autoReconnect() == false)
    {
        return ComBytes();
    }

    struct nfsfh* f_remote = NULL;
    int ret = nfs_open((struct nfs_context*)nfs_ctx, PATH_FROM_DOS(file_path_remote).c_str(), O_RDONLY, &f_remote);
    if(ret != 0)
    {
        LOG_E("failed to open remote file:%s", PATH_FROM_DOS(file_path_remote).c_str());
        return ComBytes();
    }

    ComBytes bytes;
    uint8 buf[1024];
    int read_size = 0 ;
    while((read_size = nfs_read((struct nfs_context*)nfs_ctx, f_remote, sizeof(buf), buf)) > 0)
    {
        bytes.append(buf, read_size);
    }
    nfs_close((struct nfs_context*)nfs_ctx, f_remote);
    return bytes;
}

std::map<std::string, int> ComexNfs::ls(const char* dir_path, bool full_path)
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

    std::string dir_path_str = dir_path;
    struct nfsdir* fp_dir = NULL;
    int ret = nfs_opendir((struct nfs_context*)nfs_ctx, PATH_FROM_DOS(dir_path).c_str(), &fp_dir);//nfs库不支持\分隔
    if(ret != 0)
    {
        LOG_E("failed to open remote fp_dir:%s\n", dir_path_str.c_str());
        return list;
    }

    if(full_path)
    {
        if(com_string_end_with(dir_path_str.c_str(), PATH_DELIM_STR) == false)
        {
            dir_path_str += PATH_DELIM_STR;
        }
    }
    struct nfsdirent* ent = NULL;
    while((ent = nfs_readdir((struct nfs_context*)nfs_ctx, fp_dir)) != NULL)
    {
        if(ent->name == NULL || com_string_equal(ent->name, ".") || com_string_equal(ent->name, ".."))
        {
            continue;
        }
        switch(ent->type)
        {
            case NF3LNK:
                list[dir_path_str + ent->name] = FILE_TYPE_LINK;
                break;
            case NF3REG:
                list[dir_path_str + ent->name] = FILE_TYPE_FILE;
                break;
            case NF3DIR:
                list[dir_path_str + ent->name] = FILE_TYPE_DIR;
                break;
            default:
                list[dir_path_str + ent->name] = FILE_TYPE_UNKNOWN;
                break;
        }
    }
    nfs_closedir((struct nfs_context*)nfs_ctx, fp_dir);
    return list;
}

bool ComexNfs::rename(const char* file_path, const char* file_path_new)
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

    return nfs_rename((struct nfs_context*)nfs_ctx, PATH_FROM_DOS(file_path).c_str(), PATH_FROM_DOS(file_path_new).c_str()) == 0;
}

bool ComexNfs::clean(const char* file_path)
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

    return nfs_truncate((struct nfs_context*)nfs_ctx, PATH_FROM_DOS(file_path).c_str(), 0) == 0;
}

bool ComexNfs::remove(const char* file_path)
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

    return nfs_unlink((struct nfs_context*)nfs_ctx, PATH_FROM_DOS(file_path).c_str()) == 0;
}

bool ComexNfs::mkdir(const char* dir_path)
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

    std::vector<std::string> paths = com_string_split(PATH_FROM_DOS(dir_path).c_str(), "/");
    if(paths.empty())
    {
        return true;
    }

    std::string path;//nfs不支持\分隔符
    if(paths[0].empty())
    {
        path = "/";
    }
    for(size_t i = 0; i < paths.size(); i++)
    {
        if(paths[i].empty())
        {
            continue;
        }
        path.append(paths[i]);
        int file_type = getFileType(path.c_str());
        if(file_type != FILE_TYPE_NOT_EXIST)
        {
            if(file_type != FILE_TYPE_DIR)
            {
                LOG_E("file %s already exist", path.c_str());
                return false;
            }
            path += '/';
            continue;
        }
        if(nfs_mkdir((struct nfs_context*)nfs_ctx, path.c_str()) != 0)
        {
            LOG_E("make dir %s failed,path=%s", path.c_str(), dir_path);
            return false;
        }
        path += '/';
    }

    return true;
}

bool ComexNfs::rmdir(const char* dir_path)
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

    std::string dir_path_str = PATH_FROM_DOS(dir_path);
    struct nfsdir* fp_dir = NULL;
    int ret = nfs_opendir((struct nfs_context*)nfs_ctx, dir_path_str.c_str(), &fp_dir);
    if(ret != 0)
    {
        LOG_E("failed to open remote fp_dir:%s\n", dir_path_str.c_str());
        return false;
    }

    struct nfsdirent* ent = NULL;
    while((ent = nfs_readdir((struct nfs_context*)nfs_ctx, fp_dir)) != NULL)
    {
        if(strcmp(ent->name, ".") == 0 || strcmp(ent->name, "..") == 0)
        {
            continue;
        }
        std::string path = dir_path_str;
        path.append("/");
        path.append(ent->name);

        if(ent->type == NF3DIR)
        {
            this->rmdir(path.c_str());
        }
        else
        {
            nfs_unlink((struct nfs_context*)nfs_ctx, path.c_str());
        }
    }

    nfs_rmdir((struct nfs_context*)nfs_ctx, dir_path_str.c_str());
    nfs_closedir((struct nfs_context*)nfs_ctx, fp_dir);
    return true;
}

int ComexNfs::getFileType(const char* file_path_remote)
{
    if(file_path_remote == NULL)
    {
        return FILE_TYPE_NOT_EXIST;
    }

    if(autoReconnect() == false)
    {
        return FILE_TYPE_UNKNOWN;
    }

    struct nfs_stat_64 st;
    memset(&st, 0, sizeof(st));
    if(nfs_stat64((struct nfs_context*)nfs_ctx, PATH_FROM_DOS(file_path_remote).c_str(), &st) != 0)
    {
        return FILE_TYPE_NOT_EXIST;
    }

    if((st.nfs_mode & S_IFMT) == S_IFREG)
    {
        return FILE_TYPE_FILE;
    }

    if((st.nfs_mode & S_IFMT) == S_IFDIR)
    {
        return FILE_TYPE_DIR;
    }

#if !defined(_WIN32)
    if((st.nfs_mode & S_IFMT) == S_IFLNK)
    {
        return FILE_TYPE_LINK;
    }
#endif

    return FILE_TYPE_UNKNOWN;
}

int64 ComexNfs::getFileSize(const char* file_path_remote)
{
    if(file_path_remote == NULL)
    {
        return -1;
    }

    if(autoReconnect() == false)
    {
        return -1;
    }

    struct nfs_stat_64 st;
    memset(&st, 0, sizeof(st));
    if(nfs_stat64((struct nfs_context*)nfs_ctx, PATH_FROM_DOS(file_path_remote).c_str(), &st) != 0)
    {
        return -1;
    }

    return (int64)st.nfs_size;
}

uint32 ComexNfs::getFileAccessTime(const char* file_path_remote)
{
    if(file_path_remote == NULL)
    {
        return 0;
    }

    if(autoReconnect() == false)
    {
        return 00;
    }

    struct nfs_stat_64 st;
    memset(&st, 0, sizeof(st));
    if(nfs_stat64((struct nfs_context*)nfs_ctx, PATH_FROM_DOS(file_path_remote).c_str(), &st) != 0)
    {
        return 0;
    }

    return (uint32)st.nfs_atime;
}

uint32 ComexNfs::getFileModifyTime(const char* file_path_remote)
{
    if(file_path_remote == NULL)
    {
        return 0;
    }

    if(autoReconnect() == false)
    {
        return 0;
    }

    struct nfs_stat_64 st;
    memset(&st, 0, sizeof(st));
    if(nfs_stat64((struct nfs_context*)nfs_ctx, PATH_FROM_DOS(file_path_remote).c_str(), &st) != 0)
    {
        return 0;
    }

    return (uint32)st.nfs_mtime;
}

uint32 ComexNfs::getFileChangeTime(const char* file_path_remote)
{
    if(file_path_remote == NULL)
    {
        return 0;
    }

    if(autoReconnect() == false)
    {
        return 0;
    }

    struct nfs_stat_64 st;
    memset(&st, 0, sizeof(st));
    if(nfs_stat64((struct nfs_context*)nfs_ctx, PATH_FROM_DOS(file_path_remote).c_str(), &st) != 0)
    {
        return 0;
    }
    return (uint32)st.nfs_ctime;
}
