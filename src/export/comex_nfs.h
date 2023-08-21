#ifndef __COMEX_NFS_H__
#define __COMEX_NFS_H__

#include "com_base.h"

class COM_EXPORT CPPNfs
{
public:
    CPPNfs();
    virtual ~CPPNfs();
    
	CPPNfs& setShareUrlPath(const char* url);
    CPPNfs& setHost(const char* host);
    CPPNfs& setShareName(const char* share_name);
    CPPNfs& setUID(int uid);
    CPPNfs& setGID(int gid);
    CPPNfs& setVersion(int version);
    CPPNfs& seReconnect(bool reconnect);

    CPPNfs& setLocalDir(const char* dir);
    CPPNfs& setRemoteDir(const char* dir);

    bool put(const char* file_path_local);
    bool putAs(const char* file_path_local, const char* file_path_remote);

    bool get(const char* file_path_remote);
    bool getAs(const char* file_path_remote, const char* file_path_local);
    CPPBytes getBytes(const char* file_path_remote);

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

#endif /* __COMEX_NFS_H__ */

