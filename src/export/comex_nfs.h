#ifndef __COMEX_NFS_H__
#define __COMEX_NFS_H__

#include "com_base.h"

class COM_EXPORT ComexNfs
{
public:
    ComexNfs();
    virtual ~ComexNfs();
    
	ComexNfs& setShareUrlPath(const char* url);
    ComexNfs& setHost(const char* host);
    ComexNfs& setShareName(const char* share_name);
    ComexNfs& setUID(int uid);
    ComexNfs& setGID(int gid);
    ComexNfs& setVersion(int version);
    ComexNfs& seReconnect(bool reconnect);

    ComexNfs& setLocalDir(const char* dir);
    ComexNfs& setRemoteDir(const char* dir);

    bool put(const char* file_path_local);
    bool putAs(const char* file_path_local, const char* file_path_remote);

    bool get(const char* file_path_remote);
    bool getAs(const char* file_path_remote, const char* file_path_local);
    ComBytes getBytes(const char* file_path_remote);

    std::map<std::string, int> ls(const char* dir_path, bool full_path = true);
    bool rename(const char* file_path, const char* file_path_new);
    bool clean(const char* file_path);
    bool remove(const char* file_path);
    bool mkdir(const char* dir_path);
    bool rmdir(const char* dir_path);
    int getFileType(const char* file_path_remote);
    int64 getFileSize(const char* file_path_remote);
    uint32 getFileAccessTime(const char* file_path_remote);
    uint32 getFileModifyTime(const char* file_path_remote);
    uint32 getFileChangeTime(const char* file_path_remote);
private:
    bool autoReconnect();
    void disconnect();
private:
    int uid = -1;
    int gid = -1;
    int version = 3;
    std::string host;
    std::string share_name;
    std::string url;
    bool reconnect = false;

    std::string dir_local;
    std::string dir_remote;
	
    void* nfs_ctx  = NULL;
    bool nfs_connected = false;
};
//typedef ComexNfs DEPRECATED("Use ComexNfs instead") CPPNfs;

#endif /* __COMEX_NFS_H__ */

