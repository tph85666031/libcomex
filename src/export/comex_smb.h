#ifndef __COMEX_SMB_H__
#define __COMEX_SMB_H__

#include "com_base.h"

class COM_EXPORT ComexSmb
{
public:
    ComexSmb();
    virtual ~ComexSmb();
    ComexSmb& setShareUrlPath(const char* url);
    ComexSmb& setHost(const char* host);
    ComexSmb& setShareName(const char* share);
    ComexSmb& setDomain(const char* domain);
    ComexSmb& setUsername(const char* user);
    ComexSmb& setPassword(const char* pwd);
    ComexSmb& setWorkstation(const char* workstation);
    ComexSmb& setLocalDir(const char* dir);
    ComexSmb& setRemoteDir(const char* dir);

    bool put(const char* file_path_local);
    bool putAs(const char* file_path_local, const char* file_path_remote);
    bool get(const char* file_path_remote);
    bool getAs(const char* file_path_remote, const char* file_path_local);
    ComBytes getBytes(const char* file_path_remote);

    std::map<std::string, int> ls(const char* dir_path, bool full_path = true);
    bool rename(const char* file_path, const char* file_path_new);
    bool clean(const char* file_path);
    bool remove(const char* file_path);
    bool mkdir(const char* file_path);
    bool rmdir(const char* file_path);
    int getFileType(const char* file_path_remote);
    int64 getFileSize(const char* file_path_remote);
    uint32 getFileAccessTime(const char* file_path_remote);
    uint32 getFileModifyTime(const char* file_path_remote);
    uint32 getFileChangeTime(const char* file_path_remote);
private:
    bool autoReconnect();
    void disconnect();
private:
    std::string host;
    std::string share_name;
    std::string domain;
    std::string username;
    std::string password;
    std::string workstation;
    std::string url;

    std::string dir_local;
    std::string dir_remote;

    void* smb_ctx  = NULL;
};
typedef ComexSmb DEPRECATED("Use ComexSmb instead") CPPSmb;

#endif /* __COMEX_SMB_H__ */

