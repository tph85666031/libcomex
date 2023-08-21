#ifndef __COMEX_CURL_H__
#define __COMEX_CURL_H__

#include <string>
#include <stdarg.h>
#include "com_base.h"

typedef struct
{
    long code = 400;
    std::string msg;
} HttpResponse;

COM_EXPORT int comex_curl_global_init();
COM_EXPORT void comex_curl_global_uninit();

class COM_EXPORT CurlFormFileDesc
{
public:
    std::string file;
    int64 offset = 0;
    int64 size = -1;
};

class COM_EXPORT CurlProgress
{
public:
    CurlProgress();
    ~CurlProgress();
    void setTotalSize(int64 size);
    void setCurrentSizeRemain(int64 size);
    void setCurrentSizeBase(int64 cur_size_base);
    int64 getTotalSize();
    int64 getCurrentSize();
    int getProgress();
private:
    int64 total_size = 0;
    int64 cur_size_remain = 0;
    int64 cur_size_base = 0;
    std::mutex mutex_progress;
};
/*
    CURLE_OK = 0, 0: no error
    CURLE_UNSUPPORTED_PROTOCOL, 1: unsupported protocol
    CURLE_FAILED_INIT, 2: failed init
    CURLE_URL_MALFORMAT, 3: URL using bad/illegal format or missing URL
    CURLE_URL_MALFORMAT_USER, 4: unknown error
    CURLE_COULDNT_RESOLVE_PROXY, 5: couldn’t resolve proxy name
    CURLE_COULDNT_RESOLVE_HOST, 6: couldn’t resolve host name
    CURLE_COULDNT_CONNECT, 7: couldn’t connect to server
    CURLE_FTP_WEIRD_SERVER_REPLY, 8: FTP: weird server reply
    CURLE_FTP_ACCESS_DENIED,9
    CURLE_FTP_USER_PASSWORD_INCORRECT, 10: unknown error
    CURLE_FTP_WEIRD_PASS_REPLY, 11: FTP: unknown PASS reply
    CURLE_FTP_WEIRD_USER_REPLY, 12: FTP: unknown USER reply
    CURLE_FTP_WEIRD_PASV_REPLY, 13: FTP: unknown PASV reply
    CURLE_FTP_WEIRD_227_FORMAT, 14: FTP: unknown 227 response format
    CURLE_FTP_CANT_GET_HOST, 15: FTP: can’t figure out the host in the PASV response
    CURLE_FTP_CANT_RECONNECT, 16: FTP: can’t connect to server the response code is unknown
    CURLE_FTP_COULDNT_SET_BINARY, 17: FTP: couldn’t set binary mode
    CURLE_PARTIAL_FILE, 18: Transferred a partial file
    CURLE_FTP_COULDNT_RETR_FILE, 19: FTP: couldn’t retrieve (RETR failed) the specified file
    CURLE_FTP_WRITE_ERROR, 20: FTP: the post-transfer acknowledge response was not OK
    CURLE_FTP_QUOTE_ERROR, 21: FTP: a quote command returned error
    CURLE_HTTP_RETURNED_ERROR, 22: HTTP response code said error
    CURLE_WRITE_ERROR, 23: failed writing received data to disk/application
    CURLE_MALFORMAT_USER, 24: unknown error
    CURLE_UPLOAD_FAILED, 25: upload failed (at start/before it took off)
    CURLE_READ_ERROR, 26: failed to open/read local data from file/application
    CURLE_OUT_OF_MEMORY, 27: out of memory
    CURLE_OPERATION_TIMEOUTED, 28: a timeout was reached
    CURLE_FTP_COULDNT_SET_ASCII, 29: FTP could not set ASCII mode (TYPE A)
    CURLE_FTP_PORT_FAILED, 30: FTP command PORT failed
    CURLE_FTP_COULDNT_USE_REST, 31: FTP command REST failed
    CURLE_FTP_COULDNT_GET_SIZE, 32: FTP command SIZE failed
    CURLE_HTTP_RANGE_ERROR, 33: a range was requested but the server did not deliver it
    CURLE_HTTP_POST_ERROR, 34: internal problem setting up the POST
    CURLE_SSL_CONNECT_ERROR, 35: SSL connect error
    CURLE_BAD_DOWNLOAD_RESUME, 36: couldn’t resume download
    CURLE_FILE_COULDNT_READ_FILE, 37: couldn’t read a file:// file
    CURLE_LDAP_CANNOT_BIND, 38: LDAP: cannot bind
    CURLE_LDAP_SEARCH_FAILED, 39: LDAP: search failed
    CURLE_LIBRARY_NOT_FOUND, 40: a required shared library was not found
*/
class COM_EXPORT CPPCurl
{
public:
    CPPCurl();
    virtual ~CPPCurl();
    HttpResponse post(const char* url, const char* body = NULL);
    HttpResponse get(const char* fmt, ...) __attribute__((format(printf, 2, 3)));
    HttpResponse put(const char* url, const char* body = NULL);
    HttpResponse send(const char* url, const char* method, const void* body = NULL, int body_size = 0);
    CPPCurl& addHeader(const char* header);
    CPPCurl& addHeader(const char* key, const char* value);
    CPPCurl& addFormData(const char* key, const char* value);
    CPPCurl& addFormFile(const char* name, const char* path, int64 offset = 0, int64 size = -1);
    CPPCurl& enableDebug();
    CPPCurl& disableDebug();

    CPPCurl& setVerifyCertDNS(bool enable);
    CPPCurl& setVerifyCertCA(bool enable);
    CPPCurl& setConnectionTimeout(int timeout_ms);
    CPPCurl& setReceiveTimeout(int timeout_ms);
    CPPCurl& setRetryCount(int retryCount);

    CPPCurl& setCAFile(const char* file);
    CPPCurl& setCertFile(const char* file, const char* type = NULL);
    CPPCurl& setCertPassword(const char* password);
    CPPCurl& setKeyFile(const char* file, const char* type = NULL);
    CPPCurl& setKeyPassword(const char* password);
    CPPCurl& setUsername(const char* username);
    CPPCurl& setPassword(const char* password);

    int64 getRemoteFileSize(const char* url);

    HttpResponse upload(const char* url, const char* method, const char* file_path);
    /**
        下载指定分片到内存
        @param url:下载地址
        @param remoteFileSize:远端文件总大小
        @param bytes:本地内存缓存
        @param beginByte:起始字节偏移
        @param endByte:结束字节偏移（含）
        @param progress:此分片下载进度
        @return -100:下载失败
        @return -1至-99:对应CURL错误码
        @return >0:已下载字节数
    */

    int64 download(const char* url, int64 remoteFileSize, CPPBytes& bytes,
                   int64 beginByte, int64 endByte, CurlProgress& progress);
    /**
        下载数据到内存
        @param url:下载地址
        @param bytes:本地内存缓存
        @return -100:下载失败
        @return -1至-99:对应CURL错误码
        @return >0:已下载字节数
    */
    int64 download(const char* url, CPPBytes& bytes);
    int64 download(const char* url, CPPBytes& bytes, CurlProgress& progress);

    /**
        下载数据到文件
        @param url:下载地址
        @param filePath:本地存储文件
        @return -100:下载失败
        @return -1至-99:对应CURL错误码
        @return >0:已下载字节数
    */
    int64 download(const char* url, const char* filePath);
    int64 download(const char* url, const char* filePath, CurlProgress& progress);
    static std::string ConvertErrMessage(int err_code);
private:
    void prepareCert(void* curl);
private:
    std::vector<std::string> headers;
    std::map<std::string, std::string> form_datas;
    std::map<std::string, CurlFormFileDesc> form_files;
    bool debug_enable = false;
    bool verify_cert_dns = false;
    bool verify_cert_ca = false;
    int connection_timeout_ms;
    int timeout_ms;
    int retry_count;

    std::string ca_file;
    std::string ca_path;
    std::string cert_file;
    std::string cert_password;
    std::string key_file;
    std::string key_password;

    std::string username;
    std::string password;

    std::string cert_type;
    std::string key_type;

    std::string server_cert_pem;
};

#endif /* __COMEX_CURL_H__ */

