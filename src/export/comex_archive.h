#ifndef __COMEX_ACHIVE_H__
#define __COMEX_ACHIVE_H__

#include "com_base.h"

class COM_EXPORT ArchiveZip
{
public:
    ArchiveZip();
    virtual ~ArchiveZip();

    void setPassword(const char* pwd);

    bool open(const char* file, bool append = true, bool create = true);
    bool open(const CPPBytes& content);
    void close(bool save = true);

    void clear();

    bool addFile(const char* path, const char* file);
    bool addToFile(const char* path, const void* data, int data_size);
    bool addDirectory(const char* path, const char* dir, bool recursion = false);

    int64 getFileSize(const char* path);
    std::vector<std::string> getFileList();
    bool removeFile(const char* path);
    bool removeDirectoy(const char* path);
    CPPBytes read(const char* path);
    bool readTo(const char* path, const char* to, bool append = false);
    bool extractTo(const char* dir);
private:
    void* ctx;
    std::string pwd;
};

class COM_EXPORT ArchiveTar
{
public:
    ArchiveTar();
    virtual ~ArchiveTar();

    bool open(const char* file, bool read_mode = true);
    void close();

    bool addFile(const char* path, const char* file);
    bool addDirectory(const char* path, const char* dir);
    bool extractTo(const char* dir);
private:
    bool compress(const char* file_from, const char* file_to, int type);
    bool decompress(const char* file_from, const char* file_to, int type);
private:
    void* ctx;
    bool file_added;
    bool read_mode;
    std::string file;
    std::string file_tar;
    int compress_type;
};

class COM_EXPORT ArchiveReader
{
public:
    ArchiveReader(const char* file, const char* pwd = NULL);
    virtual ~ArchiveReader();

    bool open();
    void close();
    bool isFileExist(const char* path);
    int64 getFileSize(const char* path);
    std::set<std::string> list(const char* path);
    CPPBytes read(const char* path);
    bool readTo(const char* path, const char* to);

private:
    void* ctx;
    std::string file;
    std::string pwd;
};

class COM_EXPORT ArchiveWriter
{
public:
    ArchiveWriter(const char* file, const char* pwd = NULL);
    virtual ~ArchiveWriter();

    void close();
    
    bool addFile(const char* path, const char* file);
    bool addDirectory(const char* path, const char* dir, bool recursion = false);

private:
    void* ctx;
    std::string file;
};
#endif /* __COMEX_ACHIVE_H__ */

