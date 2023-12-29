#include <zip.h>
#include <zlib.h>
#include <bzlib.h>
#include <fcntl.h>
#if !defined(_WIN32) && !defined(_WIN64)
#include <libtar.h>
#endif
#include <archive.h>
#include <archive_entry.h>

#include "com_log.h"
#include "com_file.h"
#include "comex_archive.h"

#define COMPRESS_TYPE_UNKNOWN   0
#define COMPRESS_TYPE_GZ        1
#define COMPRESS_TYPE_BZ2       2

#define ARCHIVE_READ_BLOCK_SIZE  (10*1024)

ArchiveZip::ArchiveZip()
{
    ctx = NULL;
}

ArchiveZip::~ArchiveZip()
{
    close();
}

void ArchiveZip::setPassword(const char* pwd)
{
    if(pwd != NULL)
    {
        this->pwd = pwd;
    }
}

bool ArchiveZip::open(const char* file, bool append, bool create)
{
    if(create == false && com_file_type(PATH_TO_LOCAL(file).c_str()) != FILE_TYPE_FILE)
    {
        return false;
    }
    int error_code = 0;
    int flag = 0;
    if(create)
    {
        flag |= ZIP_CREATE;
    }
    ctx = zip_open(file, flag, &error_code);
    if(ctx == NULL)
    {
        LOG_E("failed to open zip file, code=%d", error_code);
        return false;
    }
    zip_set_default_password((zip_t*)ctx, pwd.c_str());
    return true;
}

bool ArchiveZip::open(const CPPBytes& content)
{
    zip_source_t* zip_source = zip_source_buffer_create(content.getData(), content.getDataSize(), 0, NULL);
    if(zip_source == NULL)
    {
        return false;
    }
    zip_error_t error_code;
    int flag = 0;
    ctx = zip_open_from_source(zip_source, flag, &error_code);
    if(ctx == NULL)
    {
        LOG_E("failed to open zip from content, code=%d", error_code.zip_err);
        return false;
    }
    zip_set_default_password((zip_t*)ctx, pwd.c_str());
    return true;
}

void ArchiveZip::close(bool save)
{
    if(ctx == NULL)
    {
        return;
    }
    if(save)
    {
        zip_close((zip_t*)ctx);
    }
    else
    {
        zip_discard((zip_t*)ctx);
    }
    ctx = NULL;
}

void ArchiveZip::clear()
{
    if(ctx == NULL)
    {
        return;
    }
    zip_int64_t count = zip_get_num_entries((zip_t*)ctx, 0);
    for(zip_int64_t i = 0; i < count; i++)
    {
        zip_delete((zip_t*)ctx, i);
    }
}

bool ArchiveZip::addFile(const char* path, const char* file)
{
    if(ctx == NULL || path == NULL || file == NULL)
    {
        return false;
    }
    zip_source_t* s = zip_source_file((zip_t*)ctx, file, 0, 0);
    if(s == NULL)
    {
        LOG_E("failed to load file:%s", file);
        return false;
    }
    if(zip_file_add((zip_t*)ctx, path, s, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8) < 0)
    {
        LOG_E("ailed to add data to zip");
        zip_source_free(s);
        return false;
    }
    return true;
}

bool ArchiveZip::addToFile(const char* path, const void* data, int data_size)
{
    if(ctx == NULL || path == NULL || data == NULL || data_size <= 0)
    {
        return false;
    }
    zip_source_t* s = zip_source_buffer((zip_t*)ctx, data, data_size, 0);
    if(s == NULL)
    {
        LOG_E("failed to load data");
        return false;
    }
    if(zip_file_add((zip_t*)ctx, path, s, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8) < 0)
    {
        LOG_E("ailed to add data to zip");
        zip_source_free(s);
        return false;
    }
    return true;
}

bool ArchiveZip::addDirectory(const char* path, const char* dir, bool recursion)
{
    if(ctx == NULL || path == NULL || dir == NULL)
    {
        return false;
    }
    std::map<std::string, int> list;
    if(com_dir_list(PATH_TO_LOCAL(dir).c_str(), list, recursion) == false)
    {
        return false;
    }

    for(auto it = list.begin(); it != list.end(); it++)
    {
        if(it->second == FILE_TYPE_UNKNOWN || it->second == FILE_TYPE_DIR)
        {
            continue;
        }
        std::string path_internal = it->first;
        com_string_replace(path_internal, dir, path);
        if(path_internal.front() == PATH_DELIM_CHAR)
        {
            path_internal.erase(0, 1);
        }
        if(addFile(path_internal.c_str(), it->first.c_str()) == false)
        {
            return false;
        }
    }

    return true;
}

std::vector<std::string> ArchiveZip::getFileList()
{
    if(ctx == NULL)
    {
        return std::vector<std::string>();
    }
    std::vector<std::string> list;
    zip_stat_t st;
    zip_stat_init(&st);
    zip_int64_t count = zip_get_num_entries((zip_t*)ctx, 0);
    for(zip_int64_t i = 0; i < count; i++)
    {
        if(zip_stat_index((zip_t*)ctx, i, 0, &st) != 0)
        {
            LOG_I("failed");
            continue;
        }
        if((st.valid & ZIP_STAT_NAME) == 0)
        {
            LOG_I("failed");
            continue;
        }
        if(st.name == NULL)
        {
            LOG_I("failed");
            continue;
        }
        list.push_back(st.name);
    }
    return list;
}

int64 ArchiveZip::getFileSize(const char* path)
{
    if(ctx == NULL || path == NULL)
    {
        return -1;
    }
    zip_stat_t st;
    zip_stat_init(&st);
    int ret = zip_stat((zip_t*)ctx, path, 0, &st);//zip_stat在WIN中不支持/分隔
    if(ret != 0)
    {
        zip_error_t* error = zip_get_error((zip_t*)ctx);
        LOG_E("failed,ret=%d,ez=%d,es=%d,path=%s", ret, zip_error_code_zip(error), zip_error_code_system(error), path);
        return -1;
    }
    if(st.valid & ZIP_STAT_SIZE)
    {
        return st.size;
    }
    return -1;
}

bool ArchiveZip::removeFile(const char* path)
{
    if(ctx == NULL || path == NULL)
    {
        return false;
    }
    zip_int64_t index = zip_name_locate((zip_t*)ctx, path, 0);
    if(index <= 0)
    {
        return false;
    }
    if(zip_delete((zip_t*)ctx, index) < 0)
    {
        return false;
    }
    return true;
}

bool ArchiveZip::removeDirectoy(const char* path)
{
    if(ctx == NULL || path == NULL)
    {
        return false;
    }
    zip_stat_t st;
    zip_stat_init(&st);
    zip_int64_t count = zip_get_num_entries((zip_t*)ctx, 0);
    for(zip_int64_t i = 0; i < count; i++)
    {
        if(zip_stat_index((zip_t*)ctx, i, 0, &st) != 0)
        {
            return false;
        }
        if((st.valid & ZIP_STAT_NAME) && com_string_start_with(st.name, path))
        {
            zip_delete((zip_t*)ctx, i);
        }
    }
    return false;
}

CPPBytes ArchiveZip::read(const char* path)
{
    if(ctx == NULL || path == NULL)
    {
        return CPPBytes();
    }
    zip_file_t* f = zip_fopen((zip_t*)ctx, path, 0);
    if(f == NULL)
    {
        return CPPBytes();
    }
    CPPBytes result;
    uint8 buf[1024];
    zip_int64_t size = 0;
    while((size = zip_fread(f, buf, sizeof(buf))) > 0)
    {
        result.append(buf, (int)size);
    }
    zip_fclose(f);
    return result;
}

bool ArchiveZip::readTo(const char* path, const char* to, bool append)
{
    if(ctx == NULL || path == NULL || to == NULL)
    {
        return false;
    }
    if(path == NULL || to == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    com_dir_create(com_path_dir(PATH_TO_LOCAL(to).c_str()).c_str());
    FILE* f_to = NULL;
    if(append)
    {
        f_to = com_file_open(PATH_TO_LOCAL(to).c_str(), "ab+");
    }
    else
    {
        f_to = com_file_open(PATH_TO_LOCAL(to).c_str(), "wb+");
    }

    if(f_to == NULL)
    {
        LOG_E("failed to open file:%s", to);
        return false;
    }
    zip_file_t* f_from = zip_fopen((zip_t*)ctx, path, 0);
    if(f_from == NULL)
    {
        LOG_E("failed to open zip file:%s", path);
        com_file_close(f_to);
        return false;
    }
    CPPBytes result;
    uint8 buf[1024];
    zip_int64_t size = 0;
    while((size = zip_fread(f_from, buf, sizeof(buf))) > 0)
    {
        com_file_write(f_to, buf, (int)size);
    }
    zip_fclose(f_from);
    com_file_flush(f_to);
    com_file_close(f_to);
    return true;
}

bool ArchiveZip::extractTo(const char* dir)
{
    if(ctx == NULL || dir == NULL)
    {
        return false;
    }
    zip_stat_t st;
    zip_stat_init(&st);
    zip_int64_t count = zip_get_num_entries((zip_t*)ctx, 0);
    for(zip_int64_t i = 0; i < count; i++)
    {
        if(zip_stat_index((zip_t*)ctx, i, 0, &st) != 0)
        {
            LOG_I("failed");
            continue;
        }
        if((st.valid & ZIP_STAT_NAME) == 0)
        {
            LOG_I("failed");
            continue;
        }
        if(com_string_end_with(st.name, "/"))
        {
            continue;
        }
        std::string file = com_string_format("%s%c%s", dir, PATH_DELIM_CHAR, st.name);
        if(readTo(st.name, file.c_str()) == false)
        {
            LOG_W("extract failed:%s", st.name);
        }
    }
    return true;
}

ArchiveTar::ArchiveTar()
{
#if !defined(_WIN32) && !defined(_WIN64)
    ctx  = NULL;
    file_added = false;
    read_mode = true;
    compress_type = COMPRESS_TYPE_UNKNOWN;
#endif
}

ArchiveTar::~ArchiveTar()
{
#if !defined(_WIN32) && !defined(_WIN64)
    close();
    if(file != file_tar)
    {
        com_file_remove(PATH_TO_LOCAL(file_tar).c_str());
    }
#endif
}

bool ArchiveTar::open(const char* file, bool read_mode)
{
#if defined(_WIN32) || defined(_WIN64)
    return false;
#else
    if(file == NULL)
    {
        return false;
    }

    this->read_mode = read_mode;
    this->file = file;
    this->file_tar = file;
    if(com_string_end_with(file, ".gz"))
    {
        file_tar.erase(file_tar.end() - 3, file_tar.end());
        compress_type = COMPRESS_TYPE_GZ;
    }
    else if(com_string_end_with(file, ".bz2"))
    {
        file_tar.erase(file_tar.end() - 4, file_tar.end());
        compress_type = COMPRESS_TYPE_BZ2;
    }

    if(read_mode && decompress(file, file_tar.c_str(), compress_type) == false)
    {
        return false;
    }

    if(tar_open((TAR**)&ctx, file_tar.c_str(), NULL, read_mode ? O_RDONLY : (O_WRONLY | O_CREAT), 0644, TAR_GNU) != 0)
    {
        LOG_I("failed to open file:%s", file_tar.c_str());
        return false;
    }
    return true;
#endif
}

void ArchiveTar::close()
{
#if !defined(_WIN32) && !defined(_WIN64)
    if(ctx == NULL)
    {
        return;
    }
    if(file_added)
    {
        tar_append_eof((TAR*)ctx);
    }
    tar_close((TAR*)ctx);
    ctx = NULL;

    if(read_mode)
    {
        return;
    }

    if(compress(file_tar.c_str(), file.c_str(), compress_type) == false)
    {
        LOG_E("compress failed");
    }
    return;
#endif
}

bool ArchiveTar::compress(const char* file_from, const char* file_to, int type)
{
#if defined(_WIN32) || defined(_WIN64)
    return false;
#else
    if(file_from == NULL || file_to == NULL || file_from[0] == '\0' || file_to[0] == '\0')
    {
        return false;
    }
    if(type == COMPRESS_TYPE_GZ)
    {
        gzFile f_gz = gzopen(file.c_str(), "wb");
        if(f_gz == NULL)
        {
            LOG_E("gzopen failed");
            return false;
        }
        FILE* f_tar = com_file_open(PATH_TO_LOCAL(file_tar).c_str(), "rb+");
        if(f_tar == NULL)
        {
            LOG_E("fopen failed:%s", file.c_str());
            gzclose(f_gz);
            return false;
        }
        uint8 buf[1024];
        int size = 0;
        while((size = com_file_read(f_tar, buf, sizeof(buf))) > 0)
        {
            if(gzwrite(f_gz, buf, size) != size)
            {
                LOG_E("failed file_to gzwrite");
                com_file_close(f_tar);
                gzclose(f_gz);
                return false;
            }
        }
        gzflush(f_gz, Z_FULL_FLUSH);
        gzclose(f_gz);
        com_file_close(f_tar);
        return true;
    }

    if(type == COMPRESS_TYPE_BZ2)
    {
        BZFILE* f_bz2 = BZ2_bzopen(file.c_str(), "wb");
        if(f_bz2 == NULL)
        {
            LOG_E("gzopen failed");
            return false;
        }
        FILE* f_tar = com_file_open(PATH_TO_LOCAL(file_tar).c_str(), "rb+");
        if(f_tar == NULL)
        {
            LOG_E("fopen failed:%s", file.c_str());
            BZ2_bzclose(f_bz2);
            return false;
        }
        uint8 buf[1024];
        int size = 0;
        while((size = com_file_read(f_tar, buf, sizeof(buf))) > 0)
        {
            if(BZ2_bzwrite(f_bz2, buf, size) != size)
            {
                LOG_E("failed file_to BZ2_bzwrite");
                com_file_close(f_tar);
                BZ2_bzclose(f_bz2);
                return false;
            }
        }
        BZ2_bzflush(f_bz2);
        BZ2_bzclose(f_bz2);
        com_file_close(f_tar);
        return true;
    }

    return false;
#endif
}

bool ArchiveTar::decompress(const char* file_from, const char* file_to, int type)
{
#if defined(_WIN32) || defined(_WIN64)
    return false;
#else
    if(file_from == NULL || file_to == NULL || file_from[0] == '\0' || file_to[0] == '\0')
    {
        return false;
    }
    if(type == COMPRESS_TYPE_GZ)
    {
        gzFile f_gz = gzopen(file_from, "rb");
        if(f_gz == NULL)
        {
            return false;
        }
        FILE* f_tar = com_file_open(PATH_TO_LOCAL(file_to).c_str(), "wb+");
        if(f_tar == NULL)
        {
            gzclose(f_gz);
            return false;
        }
        uint8 buf[1024];
        int size = 0;
        while((size = gzread(f_gz, buf, sizeof(buf))) > 0)
        {
            if(com_file_write(f_tar, buf, size) != size)
            {
                com_file_close(f_tar);
                gzclose(f_gz);
                return false;
            }
        }
        com_file_flush(f_tar);
        com_file_close(f_tar);
        gzclose(f_gz);
        return true;
    }

    if(type == COMPRESS_TYPE_BZ2)
    {
        BZFILE* f_bz2 = BZ2_bzopen(file_from, "rb");
        if(f_bz2 == NULL)
        {
            return false;
        }
        FILE* f_tar = com_file_open(PATH_TO_LOCAL(file_to).c_str(), "wb+");
        if(f_tar == NULL)
        {
            BZ2_bzclose(f_bz2);
            return false;
        }
        uint8 buf[1024];
        int size = 0;
        while((size = BZ2_bzread(f_bz2, buf, sizeof(buf))) > 0)
        {
            if(com_file_write(f_tar, buf, size) != size)
            {
                com_file_close(f_tar);
                BZ2_bzclose(f_bz2);
                return false;
            }
        }
        com_file_flush(f_tar);
        com_file_close(f_tar);
        BZ2_bzclose(f_bz2);
        return true;
    }

    return false;
#endif
}

bool ArchiveTar::addFile(const char* path, const char* file)
{
#if defined(_WIN32) || defined(_WIN64)
    return false;
#else
    if(ctx == NULL || path == NULL || file == NULL)
    {
        return false;
    }
    if(tar_append_file((TAR*)ctx, file, path) != 0)
    {
        LOG_E("failed");
        return false;
    }
    file_added = true;
    return true;
#endif
}

bool ArchiveTar::addDirectory(const char* path, const char* dir)
{
#if defined(_WIN32) || defined(_WIN64)
    return false;
#else
    if(ctx == NULL || path == NULL || dir == NULL)
    {
        return false;
    }
    if(tar_append_tree((TAR*)ctx, (char*)dir, (char*)path) != 0)
    {
        LOG_E("failed");
        return false;
    }
    return true;
#endif
}

bool ArchiveTar::extractTo(const char* dir)
{
#if defined(_WIN32) || defined(_WIN64)
    return false;
#else
    if(ctx == NULL || dir == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    return (tar_extract_all((TAR*)ctx, (char*)dir) == 0);
#endif
}

ArchiveReader::ArchiveReader(const char* file, const char* pwd)
{
    if(file != NULL)
    {
        this->file = file;
    }
    if(pwd != NULL)
    {
        this->pwd = pwd;
    }
    content_size = 0;
    content = NULL;
}

ArchiveReader::ArchiveReader(const uint8* content, int content_size, const char* pwd)
{
    if(content != NULL && content_size > 0)
    {
        this->content = content;
        this->content_size = content_size;
    }
    if(pwd != NULL)
    {
        this->pwd = pwd;
    }
}

ArchiveReader::~ArchiveReader()
{
    close();
}

bool ArchiveReader::open()
{
    ctx = archive_read_new();
    if(ctx == NULL)
    {
        return false;
    }

    archive_read_support_filter_all((struct archive*)ctx);
    archive_read_support_format_all((struct archive*)ctx);
    if(pwd.empty() == false)
    {
        archive_read_add_passphrase((struct archive*)ctx, pwd.c_str());
    }

    int ret = ARCHIVE_FATAL;
    if(content != NULL && content_size > 0)
    {
        ret = archive_read_open_memory((struct archive*)ctx, content, content_size);
    }
    else if(file.empty() == false)
    {
        ret = archive_read_open_filename((struct archive*)ctx, file.c_str(), ARCHIVE_READ_BLOCK_SIZE);
    }
    if(ret != ARCHIVE_OK)
    {
        LOG_E("open failed,ret=%d,file=%s", ret, file.c_str());
        archive_read_free((struct archive*)ctx);
        ctx = NULL;
        return false;
    }
    return true;
}

void ArchiveReader::close()
{
    if(ctx != NULL)
    {
        archive_read_free((struct archive*)ctx);
        ctx = NULL;
    }
}

bool ArchiveReader::isFileExist(const char* path)
{
    if(path == NULL || open() == false)
    {
        return false;
    }
    bool found = false;
    struct archive_entry* entry = NULL;
    while(archive_read_next_header((struct archive*)ctx, &entry) == ARCHIVE_OK)
    {
        const char* path_cur = archive_entry_pathname(entry);
        if(path_cur == NULL)
        {
            continue;
        }
        std::string path_cur_str = path_cur;
        if(com_string_start_with(path_cur_str.c_str(), "./")
                || com_string_start_with(path_cur_str.c_str(), ".\\"))
        {
            path_cur_str.erase(0, 2);
        }
        if(path_cur_str == path)
        {
            found = true;
            break;
        }
    }
    close();
    return found;
}

int64 ArchiveReader::getFileSize(const char* path)
{
    if(path == NULL || open() == false)
    {
        return -1;
    }
    std::string path_str = path;
    if(com_string_start_with(path_str.c_str(), "." PATH_DELIM_STR))
    {
        path_str.erase(0, 2);
    }
    int64 size = 0;
    struct archive_entry* entry = NULL;
    while(archive_read_next_header((struct archive*)ctx, &entry) == ARCHIVE_OK)
    {
        const char* path_cur = archive_entry_pathname(entry);
        if(path_cur == NULL)
        {
            continue;
        }
        std::string path_cur_str = PATH_TO_LOCAL(path_cur);
        if(path_str == path_cur_str)
        {
            if(archive_entry_size_is_set(entry))
            {
                size = archive_entry_size(entry);
            }
            break;
        }
    }
    close();
    return size;
}

std::vector<std::string> ArchiveReader::list(const char* path)
{
    if(path == NULL || open() == false)
    {
        return std::vector<std::string>();
    }
    std::vector<std::string> results;
    struct archive_entry* entry = NULL;
    while(archive_read_next_header((struct archive*)ctx, &entry) == ARCHIVE_OK)
    {
        const char* path_cur = archive_entry_pathname(entry);
        if(path_cur == NULL)
        {
            continue;
        }
        std::string path_cur_str = PATH_TO_LOCAL(path_cur);
        if(com_string_match(path_cur_str.c_str(), path))
        {
            results.push_back(path_cur);
        }
    }
    close();
    return results;
}

void ArchiveReader::list(const char* path, std::function<void(const std::string&, int64)> func)
{
    if(path == NULL || open() == false)
    {
        return;
    }
    struct archive_entry* entry = NULL;
    while(archive_read_next_header((struct archive*)ctx, &entry) == ARCHIVE_OK)
    {
        const char* path_cur = archive_entry_pathname(entry);
        if(path_cur == NULL)
        {
            continue;
        }
        std::string path_cur_str = PATH_TO_LOCAL(path_cur);
        if(com_string_match(path_cur_str.c_str(), path))
        {
            func(path_cur_str, archive_entry_size(entry));
        }
    }
    close();
}

CPPBytes ArchiveReader::read()
{
    CPPBytes data;
    int64 size = 0;
    uint8 buf[4096];
    while((size = archive_read_data((struct archive*)ctx, buf, sizeof(buf))) > 0)
    {
        data.append(buf, (int)size);
    }
    return data;
}

CPPBytes ArchiveReader::read(const char* path)
{
    if(path == NULL || open() == false)
    {
        return CPPBytes();
    }
    CPPBytes data;
    struct archive_entry* entry = NULL;
    while(archive_read_next_header((struct archive*)ctx, &entry) == ARCHIVE_OK)
    {
        const char* path_cur = archive_entry_pathname(entry);
        if(path_cur == NULL)
        {
            continue;
        }
        std::string path_cur_str = PATH_TO_LOCAL(path_cur);
        if(path_cur_str == path
                && archive_entry_size_is_set(entry)
                && archive_entry_size(entry) > 0)
        {
            int64 size = 0;
            uint8 buf[4096];
            data.reserve(archive_entry_size(entry));
            while((size = archive_read_data((struct archive*)ctx, buf, sizeof(buf))) > 0)
            {
                data.append(buf, (int)size);
            }
            break;
        }
    }
    close();
    return data;
}

bool ArchiveReader::readTo(const char* path, const char* to)
{
    if(path == NULL || to == NULL || open() == false)
    {
        return false;
    }
    struct archive_entry* entry = NULL;
    while(archive_read_next_header((struct archive*)ctx, &entry) == ARCHIVE_OK)
    {
        const char* path_cur = archive_entry_pathname(entry);
        if(path_cur == NULL)
        {
            continue;
        }
        std::string path_cur_str = PATH_TO_LOCAL(path_cur);
        if(path_cur_str == path
                && archive_entry_size_is_set(entry)
                && archive_entry_size(entry) > 0)
        {
            FILE* f = com_file_open(PATH_TO_LOCAL(to).c_str(), "wb+");
            if(f != NULL)
            {
                int64 size = 0;
                uint8 buf[4096];
                while((size = archive_read_data((struct archive*)ctx, buf, sizeof(buf))) > 0)
                {
                    com_file_write(f, buf, (int)size);
                }
                com_file_flush(f);
                com_file_close(f);
                close();
                return true;
            }
        }
    }
    close();
    return false;
}

bool ArchiveReader::extractTo(const char* dir)
{
    if(dir == NULL || open() == false)
    {
        return false;
    }
    struct archive_entry* entry = NULL;
    while(archive_read_next_header((struct archive*)ctx, &entry) == ARCHIVE_OK)
    {
        const char* path_cur = archive_entry_pathname(entry);
        if(path_cur == NULL || archive_entry_size(entry) <= 0)
        {
            continue;
        }
        std::string file_target = PATH_TO_LOCAL(com_string_format("%s/%s", dir, path_cur));
        if(file_target.back() == '/')
        {
            continue;
        }
        com_dir_create(com_path_dir(file_target.c_str()).c_str());
        FILE* fp = com_file_open(file_target.c_str(), "w+");
        if(fp == NULL)
        {
            continue;
        }
        int64 size = 0;
        uint8 buf[4096];
        while((size = archive_read_data((struct archive*)ctx, buf, sizeof(buf))) > 0)
        {
            com_file_write(fp, buf, size);
        }
        com_file_flush(fp);
        com_file_close(fp);
    }
    close();
    return true;
}

ArchiveWriter::ArchiveWriter(const char* file, const char* pwd)
{
    if(file == NULL)
    {
        return;
    }
    ctx = archive_write_new();
    if(ctx == NULL)
    {
        LOG_E("failed");
        return;
    }
    if(com_string_end_with(file, ".tar.gz"))
    {
        archive_write_set_format_ustar((struct archive*)ctx);
        archive_write_add_filter_gzip((struct archive*)ctx);
    }
    else if(com_string_end_with(file, ".tar.bz2"))
    {
        archive_write_set_format_ustar((struct archive*)ctx);
        archive_write_add_filter_bzip2((struct archive*)ctx);
    }
    else if(com_string_end_with(file, ".tar.xz"))
    {
        archive_write_set_format_ustar((struct archive*)ctx);
        archive_write_add_filter_xz((struct archive*)ctx);
    }
    else if(com_string_end_with(file, ".zip"))
    {
        archive_write_set_format_zip((struct archive*)ctx);
    }
    else
    {
        LOG_E("file not supporrt:%s", file);
        return;
    }

    if(pwd != NULL)
    {
        archive_write_set_passphrase((struct archive*)ctx, pwd);
    }

    if(archive_write_open_filename((struct archive*)ctx, file) != ARCHIVE_OK)
    {
        LOG_E("open failed:%s", archive_error_string((struct archive*)ctx));
        return;
    }
}

ArchiveWriter::ArchiveWriter(CPPBytes& buffer, const char* suffix, const char* pwd)
{
    if(suffix == NULL)
    {
        return;
    }
    ctx = archive_write_new();
    if(ctx == NULL)
    {
        LOG_E("failed");
        return;
    }
    if(com_string_equal(suffix, "tar.gz"))
    {
        archive_write_set_format_ustar((struct archive*)ctx);
        archive_write_add_filter_gzip((struct archive*)ctx);
    }
    else if(com_string_equal(suffix, "tar.bz2"))
    {
        archive_write_set_format_ustar((struct archive*)ctx);
        archive_write_add_filter_bzip2((struct archive*)ctx);
    }
    else if(com_string_equal(suffix, "tar.xz"))
    {
        archive_write_set_format_ustar((struct archive*)ctx);
        archive_write_add_filter_xz((struct archive*)ctx);
    }
    else if(com_string_equal(suffix, "zip"))
    {
        archive_write_set_format_zip((struct archive*)ctx);
    }
    else
    {
        LOG_E("suffix not support:%s", suffix);
        return;
    }

    if(pwd != NULL)
    {
        archive_write_set_passphrase((struct archive*)ctx, pwd);
    }

    if(archive_write_open((struct archive*)ctx, &buffer,
                          (int (*)(struct archive*, void*))MemOpen,
                          (ssize_t (*)(struct archive*, void*, const void*, size_t))MemWrite,
                          (int (*)(struct archive*, void*))MemClose) != ARCHIVE_OK)
    {
        LOG_E("open failed:%s", archive_error_string((struct archive*)ctx));
        return ;
    }
}

ArchiveWriter::~ArchiveWriter()
{
    close();
}

void ArchiveWriter::close()
{
    if(ctx != NULL)
    {
        archive_write_free((struct archive*)ctx);
        ctx = NULL;
    }
}

bool ArchiveWriter::addFile(const char* path, const char* file)
{
    if(ctx == NULL || path == NULL || file == NULL)
    {
        return false;
    }
    struct stat statbuff;
    if(stat(file, &statbuff) != 0)
    {
        return false;
    }
    FILE* f = com_file_open(PATH_TO_LOCAL(file).c_str(), "rb");
    if(f == NULL)
    {
        return false;
    }
    struct archive_entry* entry = archive_entry_new();
    if(entry == NULL)
    {
        com_file_close(f);
        return false;
    }

    archive_entry_copy_stat(entry, &statbuff);
    archive_entry_set_pathname(entry, path);
    archive_entry_set_size(entry, statbuff.st_size);
    int ret = archive_write_header((struct archive*)ctx, entry);
    if(ret != ARCHIVE_OK)
    {
        LOG_E("write failed,ret=%d:%s,path=%s", ret, archive_error_string((struct archive*)ctx), path);
        com_file_close(f);
        archive_entry_free(entry);
        return false;
    }
    int64 size = 0;
    uint8 buf[4096];
    int64 size_writted = 0;
    while((size = com_file_read(f, buf, sizeof(buf))) > 0)
    {
        size_writted += (int64)archive_write_data((struct archive*)ctx, buf, (size_t)size);
    }
    com_file_close(f);
    archive_write_finish_entry((struct archive*)ctx);
    archive_entry_free(entry);
    return (size_writted == (int64)statbuff.st_size);
}

bool ArchiveWriter::addDirectory(const char* path, const char* dir, bool recursion)
{
    if(ctx == NULL || path == NULL || dir == NULL)
    {
        return false;
    }
    std::map<std::string, int> list;
    if(com_dir_list(dir, list, recursion) == false)
    {
        return false;
    }

    for(auto it = list.begin(); it != list.end(); it++)
    {
        if(it->second == FILE_TYPE_UNKNOWN || it->second == FILE_TYPE_DIR)
        {
            continue;
        }
        std::string file = it->first;
        com_string_replace(file, dir, "");
        if(file.front() == PATH_DELIM_CHAR)
        {
            file.erase(0, 1);
        }
        std::string path_internal = path;
        if(path_internal.back() == PATH_DELIM_CHAR)
        {
            path_internal.pop_back();
        }

        if(path_internal.empty())
        {
            path_internal = file;
        }
        else
        {
            path_internal += PATH_DELIM_STR + file;
        }
        addFile(path_internal.c_str(), it->first.c_str());
    }
    return true;
}

int ArchiveWriter::MemOpen(void* ctx, CPPBytes* buffer)
{
    if(ctx == NULL || buffer == NULL)
    {
        return ARCHIVE_FATAL;
    }
    if(archive_write_get_bytes_in_last_block((struct archive*)ctx) == -1)
    {
        archive_write_set_bytes_in_last_block((struct archive*)ctx, 1);
    }
    return ARCHIVE_OK;
}

ssize_t ArchiveWriter::MemWrite(void* ctx, CPPBytes* buffer, const void* buff, size_t size)
{
    if(ctx == NULL || buffer == NULL)
    {
        return ARCHIVE_FATAL;
    }

    buffer->append((uint8*)buff, size);
    return size;
}

int ArchiveWriter::MemClose(void* ctx, CPPBytes* buffer)
{
    if(ctx == NULL || buffer == NULL)
    {
        return ARCHIVE_FATAL;
    }
    return ARCHIVE_OK;
}

