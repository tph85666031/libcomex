#include <string>
#include <vector>
#include <curl/curl.h>
#include "com_base.h"
#include "comex_curl.h"
#include "com_log.h"
#include "com_file.h"

static size_t mime_file_readfunc(char* buffer, size_t size, size_t nitems, void* arg)
{
    if(buffer == NULL || size * nitems <= 0 || arg == NULL)
    {
        return CURL_READFUNC_ABORT;
    }
    int64 ret = com_file_read((FILE*)arg, buffer, size * nitems);
    if(ret >= 0)
    {
        return ret;
    }
    return 0;
}

static int mime_file_seekfunc(void* arg, curl_off_t offset, int origin)
{
    if(arg == NULL)
    {
        return CURL_SEEKFUNC_CANTSEEK;
    }
    if(origin == SEEK_SET)
    {
        if(com_file_seek_set((FILE*)arg, offset))
        {
            return CURL_SEEKFUNC_OK;
        }
    }
    else if(origin == SEEK_CUR)
    {
        return com_file_seek_get((FILE*)arg);
    }
    else if(origin == SEEK_END)
    {
        com_file_seek_tail((FILE*)arg);
        return CURL_SEEKFUNC_OK;
    }
    return CURL_SEEKFUNC_FAIL;
}

static void mime_file_freefunc(void* arg)
{
    FILE* fd = (FILE*)arg;
    if(fd != NULL)
    {
        com_file_close(fd);
    }
}


static size_t http_null_callback(void* ptr, size_t size, size_t nmemb, void* userp)
{
    return size * nmemb;
}

static size_t http_response_callback(void* ptr, size_t size, size_t nmemb, void* userp)
{
    HttpResponse* response = (HttpResponse*)userp; //存储返回值  用于httPost 或者 httpGet返回bool
    if(response != NULL)
    {
        if(size * nmemb > 0 && ptr != NULL)
        {
            response->msg.append((char*)ptr, size * nmemb);
        }
    }
    return size * nmemb;
}

static size_t http_file_download_callback(void* ptr, size_t size, size_t nmemb, void* userp)
{
    FILE* file = (FILE*)userp;
    if(file == NULL || size * nmemb <= 0 || ptr == NULL)
    {
        return 0;
    }

    //LOG_D("file download called,size=%d", size * nmemb);
    int64 ret = com_file_write(file, ptr, (int)(size * nmemb));
    if(ret <= 0)
    {
        return 0;
    }
    com_file_flush(file);
    return ret;
}

static size_t http_memory_download_callback(void* ptr, size_t size, size_t nmemb, void* userp)
{
    ComBytes* bytes = (ComBytes*)userp;
    if(bytes == NULL || size * nmemb <= 0 || ptr == NULL)
    {
        return 0;
    }
    //LOG_D("memory download called,size=%d", size * nmemb);
    bytes->append((uint8*)ptr, (int)(size * nmemb));
    return size * nmemb;
}

static int http_download_progress(void* ptr, curl_off_t total_to_download, curl_off_t now_downloaded,
                                  curl_off_t total_to_upload, curl_off_t now_uploaded)
{
    CurlProgress* progress = (CurlProgress*)ptr;
    if(progress == NULL)
    {
        return 0;
    }
    progress->setCurrentSizeRemain((int64)now_downloaded);
    return 0;
}

int comex_curl_global_init()
{
    return curl_global_init(CURL_GLOBAL_ALL);
}

void comex_curl_global_uninit()
{
    curl_global_cleanup();
}

CurlProgress::CurlProgress()
{
}

CurlProgress::~CurlProgress()
{
}

int CurlProgress::getProgress()
{
    std::lock_guard<std::mutex> lck(mutex_progress);
    if(total_size <= 0)
    {
        return 0;
    }
    int val = (int)((cur_size_base + cur_size_remain) * 100 / total_size);
    if(val < 0)
    {
        val = 0;
    }
    else if(val > 100)
    {
        val = 100;
    }
    return val;
}

void CurlProgress::setCurrentSizeBase(int64 cur_size_base)
{
    std::lock_guard<std::mutex> lck(mutex_progress);
    if(cur_size_base > 0)
    {
        this->cur_size_base = cur_size_base;
    }
}

void CurlProgress::setTotalSize(int64 size)
{
    std::lock_guard<std::mutex> lck(mutex_progress);
    if(size > 0)
    {
        this->total_size = size;
    }
}

void CurlProgress::setCurrentSizeRemain(int64 size)
{
    std::lock_guard<std::mutex> lck(mutex_progress);
    if(size > 0)
    {
        this->cur_size_remain = size;
    }
}

int64 CurlProgress::getTotalSize()
{
    std::lock_guard<std::mutex> lck(mutex_progress);
    return total_size;
}

int64 CurlProgress::getCurrentSize()
{
    std::lock_guard<std::mutex> lck(mutex_progress);
    return (cur_size_base + cur_size_remain);
}

ComexCurl::ComexCurl()
{
    retry_count = 0;
    connection_timeout_ms = 5000L;
    timeout_ms = 0;
    debug_enable = false;
}

ComexCurl::~ComexCurl()
{
}

void ComexCurl::prepareCert(void* curl)
{
    if(curl == NULL)
    {
        return;
    }

    //证书
    curl_easy_setopt((CURL*)curl, CURLOPT_SSL_VERIFYHOST, verify_cert_dns ? 2 : 0); //验证证书的CN及域名匹配问题
    curl_easy_setopt((CURL*)curl, CURLOPT_SSL_VERIFYPEER, verify_cert_ca ? 1 : 0); //验证对端证书

    //如果CA不可信则需要手动加载CA来验证对端证书
    if(ca_file.empty() == false && ca_path.empty() == false)
    {
        curl_easy_setopt((CURL*)curl, CURLOPT_CAPATH, ca_path.c_str());
        curl_easy_setopt((CURL*)curl, CURLOPT_CAINFO, ca_file.c_str());
        curl_easy_setopt((CURL*)curl, CURLOPT_SSL_VERIFYPEER, 1);
    }

    //提供证书供服务端验证
    if(cert_file.empty() == false && key_file.empty() == false)
    {
        curl_easy_setopt((CURL*)curl, CURLOPT_SSLCERT, cert_file.c_str());
        curl_easy_setopt((CURL*)curl, CURLOPT_SSLKEY, key_file.c_str());

        if(cert_password.empty() == false)
        {
            curl_easy_setopt((CURL*)curl, CURLOPT_SSLCERTPASSWD, cert_password.c_str());
        }
        if(key_password.empty() == false)
        {
            curl_easy_setopt((CURL*)curl, CURLOPT_SSLKEYPASSWD, key_password.c_str());
        }

    }
    if(cert_type.empty() == false)
    {
        curl_easy_setopt((CURL*)curl, CURLOPT_SSLCERTTYPE, cert_type.c_str());
    }
    if(key_type.empty() == false)
    {
        curl_easy_setopt((CURL*)curl, CURLOPT_SSLKEYTYPE, key_type.c_str());
    }
}

ComexCurl& ComexCurl::addHeader(const char* header)
{
    if(header != NULL && header[0] != '\0')
    {
        headers.push_back(header);
    }
    return *this;
}

ComexCurl& ComexCurl::addHeader(const char* key, const char* value)
{
    if(key != NULL && value != NULL && key[0] != '\0' && value[0] != '\0')
    {
        headers.push_back(com_string_format("%s:%s", key, value));
    }
    return *this;
}

ComexCurl& ComexCurl::addFormData(const char* key, const char* value)
{
    if(key != NULL && value != NULL && key[0] != '\0' && value[0] != '\0')
    {
        form_datas[key] = value;
    }
    return *this;
}

ComexCurl& ComexCurl::addFormFile(const char* name, const char* path, int64 offset, int64 size)
{
    if(name != NULL && path != NULL && name[0] != '\0' && path[0] != '\0')
    {
        CurlFormFileDesc desc;
        desc.file = path;
        desc.offset = offset;
        desc.size = size;
        form_files[name] = desc;
    }
    return *this;
}

ComexCurl& ComexCurl::enableDebug()
{
    debug_enable = true;
    return *this;
}

ComexCurl& ComexCurl::disableDebug()
{
    debug_enable = false;
    return *this;
}

ComexCurl& ComexCurl::setVerifyCertDNS(bool enable)
{
    verify_cert_dns = enable;
    return *this;
}

ComexCurl& ComexCurl::setVerifyCertCA(bool enable)
{
    verify_cert_ca = enable;
    return *this;
}

ComexCurl& ComexCurl::setConnectionTimeout(int timeout_ms)
{
    connection_timeout_ms = timeout_ms;
    return *this;
}

ComexCurl& ComexCurl::setReceiveTimeout(int timeout_ms)
{
    this->timeout_ms = timeout_ms;
    return *this;
}

ComexCurl& ComexCurl::setRetryCount(int retryCount)
{
    if(retryCount >= 0)
    {
        this->retry_count = retryCount;
    }
    return *this;
}

ComexCurl& ComexCurl::setCAFile(const char* file)
{
    if(file != NULL)
    {
        this->ca_file = file;
        FilePath path(ca_file.c_str());
        this->ca_path = path.getDir();
    }
    return *this;
}

ComexCurl& ComexCurl::setCertFile(const char* file, const char* type)
{
    if(file != NULL)
    {
        this->cert_file = file;
    }
    if(type != NULL)
    {
        this->cert_type = type;
    }
    return *this;
}

ComexCurl& ComexCurl::setCertPassword(const char* password)
{
    if(password != NULL)
    {
        this->cert_password = password;
    }
    return *this;
}

ComexCurl& ComexCurl::setKeyFile(const char* file, const char* type)
{
    if(file != NULL)
    {
        this->key_file = file;
    }
    if(type != NULL)
    {
        this->key_type = type;
    }
    return *this;
}

ComexCurl& ComexCurl::setKeyPassword(const char* password)
{
    if(password != NULL)
    {
        this->key_password = password;
    }
    return *this;
}

ComexCurl& ComexCurl::setUsername(const char* username)
{
    if(username != NULL)
    {
        this->username = username;
    }
    return *this;
}

ComexCurl& ComexCurl::setPassword(const char* password)
{
    if(password != NULL)
    {
        this->password = password;
    }
    return *this;
}

HttpResponse ComexCurl::post(const char* url, const char* body)
{
    return send(url, "POST", body, com_string_len(body));
}

HttpResponse ComexCurl::get(const char* fmt, ...)
{
    char url[4096];
    va_list list;
    va_start(list, fmt);
    vsnprintf(url, sizeof(url), fmt, list);
    va_end(list);

    return send(url, "GET");
}

HttpResponse ComexCurl::put(const char* url, const char* body)
{
    return send(url, "PUT", body, com_string_len(body));
}

HttpResponse ComexCurl::send(const char* url, const char* method, const void* body, int body_size)
{
    HttpResponse response;
    if(url == NULL || method == NULL)
    {
        LOG_E("arg incorrent,url=%p,method=%p", url, method);
        return response;
    }
    CURL* curl = curl_easy_init();
    if(curl == NULL)
    {
        LOG_E("failed to init curl");
        return response;
    }

    struct curl_slist* curl_headers = NULL;
    for(size_t i = 0; i < headers.size() ; i++)
    {
        curl_headers = curl_slist_append(curl_headers, headers[i].c_str());
    }

    curl_mime* mime = NULL;
    if(form_datas.empty() == false || form_files.empty() == false)
    {
        mime = curl_mime_init(curl);
        if(mime == NULL)
        {
            LOG_E("failed to init curl mime");
            return response;
        }
        for(auto it = form_datas.begin(); it != form_datas.end(); it++)
        {
            curl_mimepart* part = curl_mime_addpart(mime);
            curl_mime_name(part, it->first.c_str());
            curl_mime_data(part, it->second.c_str(), CURL_ZERO_TERMINATED);
            curl_mime_type(part, "text/plain");
        }
        for(auto it = form_files.begin(); it != form_files.end(); it++)
        {
            curl_mimepart* part = curl_mime_addpart(mime);
            curl_mime_name(part, it->first.c_str());

            CurlFormFileDesc& desc = it->second;
            curl_mime_filename(part, com_path_name(desc.file.c_str()).c_str());
            FILE* fd = com_file_open(desc.file.c_str(), "rb");
            com_file_seek_set(fd, desc.offset);
            curl_mime_data_cb(part, desc.size, mime_file_readfunc, mime_file_seekfunc, mime_file_freefunc, fd);
        }
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    }

    if(username.empty() == false && password.empty() == false)
    {
        curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, connection_timeout_ms);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, debug_enable ? 1L : 0L); //打开调试日志
    curl_easy_setopt(curl, CURLOPT_CERTINFO, 1L);

    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if(body != NULL || body_size > 0)
    {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body_size);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_response_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    prepareCert(curl);

    CURLcode ret = curl_easy_perform(curl);
    if(ret == CURLE_OK)
    {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.code);
    }
    else
    {
        LOG_W("curl perform failed,ret=%d", ret);
        response.code = ret;
    }

    if(mime != NULL)
    {
        curl_mime_free(mime);
    }

    struct curl_certinfo* cert_chain = NULL;
    curl_easy_getinfo(curl, CURLINFO_CERTINFO, &cert_chain);
    if(cert_chain != NULL)
    {
        for(int i = 0; i < cert_chain->num_of_certs; i++)
        {
            struct curl_slist* slist = NULL;
            for(slist = cert_chain->certinfo[i]; slist != NULL; slist = slist->next)
            {
                if(com_string_start_with(slist->data, "Cert:"))
                {
                    server_cert_pem.clear();
                    server_cert_pem.append(slist->data + sizeof("Cert:") - 1);
                    break;
                }
            }
        }
    }

    if(curl_headers != NULL)
    {
        curl_slist_free_all(curl_headers);
    }
    headers.clear();
    form_datas.clear();
    form_files.clear();
    curl_easy_cleanup(curl);
    return response;
}

int64 ComexCurl::getRemoteFileSize(const char* url)
{
    if(url == NULL || url[0] == '\0')
    {
        LOG_E("url empty");
        return -1;
    }
    CURL* curl = curl_easy_init();
    if(curl == NULL)
    {
        LOG_E("curl init failed");
        return -1;
    }
    curl_off_t size = 0;

    struct curl_slist* curl_headers = NULL;
    for(size_t i = 0; i < headers.size() ; i++)
    {
        curl_headers = curl_slist_append(curl_headers, headers[i].c_str());
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
    curl_easy_setopt(curl, CURLOPT_HEADER, 1);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, debug_enable ? 1L : 0L); //打开调试日志
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_null_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
    prepareCert(curl);

    CURLcode ret = CURLE_OPERATION_TIMEDOUT;
    int retry = 1;
    long code = 0;
    do
    {
        ret = curl_easy_perform(curl);
        size = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

        if(ret == CURLE_OK
                && code >= 200 && code < 300
                && curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &size) == CURLE_OK)
        {
            break;
        }
        //服务端可能不支持HEAD方法取长度，折衷使用GET来获取
        curl_easy_setopt(curl, CURLOPT_HEADER, 0);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
    }
    while(retry--);

    if(curl_headers != NULL)
    {
        curl_slist_free_all(curl_headers);
    }
    curl_easy_cleanup(curl);

    if(size <= 0)
    {
        LOG_D("failed to get file size,ret=%d,code=%ld", ret, code);
        return (code == 0) ? (-1 * ret) : (-1 * code);
    }
    return (int64)size;
}

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
int64 ComexCurl::download(const char* url, int64 remote_file_size, ComBytes& bytes,
                        int64 begin_byte, int64 end_byte, CurlProgress& progress)
{
    long code = 0;
    if(url == NULL || url[0] == '\0' || begin_byte < 0 || end_byte < 0)
    {
        LOG_E("url is empty");
        return -100;
    }
    int64 local_data_size = bytes.getDataSize();
    progress.setCurrentSizeBase(local_data_size);
    progress.setTotalSize(remote_file_size);
    if(local_data_size > remote_file_size)
    {
        LOG_E("data exist");
        return -101;
    }
    if(local_data_size == remote_file_size)
    {
        LOG_W("download may already finished");
        return local_data_size;
    }

    if(begin_byte < local_data_size)
    {
        begin_byte = local_data_size;
    }
    if(end_byte < 0 || end_byte > remote_file_size)
    {
        end_byte = remote_file_size;
    }

    CURL* curl = curl_easy_init();
    if(curl == NULL)
    {
        LOG_E("cur init failed");
        return -103;
    }

    struct curl_slist* curl_headers = NULL;
    for(size_t i = 0; i < headers.size() ; i++)
    {
        curl_headers = curl_slist_append(curl_headers, headers[i].c_str());
    }
    headers.clear();

    char range_str[128];
    snprintf(range_str, sizeof(range_str), "%lld-%lld", begin_byte, end_byte);
    LOG_D("download range [%s]", range_str);
    curl_easy_setopt(curl, CURLOPT_RANGE, range_str);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &progress);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, http_download_progress);
    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, connection_timeout_ms);
    curl_easy_setopt(curl, CURLOPT_POST, 0L);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if(timeout_ms > 0)
    {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
    }
    curl_easy_setopt(curl, CURLOPT_VERBOSE, debug_enable ? 1L : 0L); //打开调试日志
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_memory_download_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &bytes);
    prepareCert(curl);

    int retry = retry_count;
    CURLcode ret = CURLE_OPERATION_TIMEOUTED;
    do
    {
        ret = curl_easy_perform(curl);
        if(ret != CURLE_OPERATION_TIMEOUTED)
        {
            break;
        }
        com_sleep_s(retry_count - retry);
    }
    while(retry--);

    if(curl_headers != NULL)
    {
        curl_slist_free_all(curl_headers);
    }

    if(ret != CURLE_OK)
    {
        LOG_E("download failed, ret=%d:%s", ret, curl_easy_strerror(ret));
        curl_easy_cleanup(curl);
        return (-1 * ret);
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    if(code < 200 || code >= 300)
    {
        curl_easy_cleanup(curl);
        LOG_E("download failed");
        return (-1 * code);
    }

    curl_easy_cleanup(curl);
    return bytes.getDataSize();
}

/**
    下载数据到内存
    @param url:下载地址
    @param bytes:本地内存缓存
    @return -100:下载失败
    @return -1至-99:对应CURL错误码
    @return >0:已下载字节数
*/
int64 ComexCurl::download(const char* url, ComBytes& bytes)
{
    CurlProgress progress;
    return download(url, bytes, progress);
}

int64 ComexCurl::download(const char* url, ComBytes& bytes, CurlProgress& progress)
{
    long code = 0;
    if(url == NULL || url[0] == '\0')
    {
        LOG_E("url is empty");
        return -100;
    }
    int64 remote_file_size = getRemoteFileSize(url);
    if(remote_file_size <= 0)
    {
        LOG_E("failed to get remote file size");
        return remote_file_size;
    }
    int64 local_data_size = bytes.getDataSize();
    if(local_data_size > remote_file_size)
    {
        LOG_E("file exist");
        return -101;
    }

    if(local_data_size == remote_file_size)
    {
        LOG_W("download may already finished");
        return local_data_size;
    }

    CURL* curl = curl_easy_init();
    if(curl == NULL)
    {
        LOG_E("cur init failed");
        return -103;
    }

    struct curl_slist* curl_headers = NULL;
    for(size_t i = 0; i < headers.size() ; i++)
    {
        curl_headers = curl_slist_append(curl_headers, headers[i].c_str());
    }
    headers.clear();

    if(local_data_size > 0)
    {
        LOG_W("data already exist, will resume the download progress");
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, local_data_size);
    }

    progress.setCurrentSizeBase(local_data_size);
    progress.setTotalSize(remote_file_size);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &progress);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, http_download_progress);
    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, connection_timeout_ms);
    curl_easy_setopt(curl, CURLOPT_POST, 0L);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if(timeout_ms > 0)
    {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
    }
    curl_easy_setopt(curl, CURLOPT_VERBOSE, debug_enable ? 1L : 0L); //打开调试日志
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_memory_download_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &bytes);
    prepareCert(curl);

    int retry = retry_count;
    CURLcode ret = CURLE_OPERATION_TIMEOUTED;
    do
    {
        ret = curl_easy_perform(curl);
        if(ret != CURLE_OPERATION_TIMEOUTED)
        {
            break;
        }
        com_sleep_s(retry_count - retry);
    }
    while(retry--);

    if(curl_headers != NULL)
    {
        curl_slist_free_all(curl_headers);
    }

    if(ret != CURLE_OK)
    {
        LOG_E("download failed, ret=%d:%s", ret, curl_easy_strerror(ret));
        curl_easy_cleanup(curl);
        return (-1 * ret);
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    if(code < 200 || code >= 300)
    {
        curl_easy_cleanup(curl);
        LOG_E("download failed");
        return (-1 * code);
    }

    curl_easy_cleanup(curl);
    return bytes.getDataSize();
}

/**
    下载数据到文件
    @param url:下载地址
    @param filePath:本地存储文件
    @return -100:下载失败
    @return -1至-99:对应CURL错误码
    @return >0:已下载字节数
*/
int64 ComexCurl::download(const char* url, const char* file_path)
{
    CurlProgress progress;
    return download(url, file_path, progress);
}

int64 ComexCurl::download(const char* url, const char* file_path, CurlProgress& progress)
{
    long code = 0;
    if(url == NULL || url[0] == '\0' || file_path == NULL || file_path[0] == '\0')
    {
        LOG_E("url or file_path is empty");
        return -100;
    }
    int64 remote_file_size = getRemoteFileSize(url);
    if(remote_file_size <= 0)
    {
        LOG_D("failed to get remote file size:%lld", remote_file_size);
        return remote_file_size;
    }
#if 0
    int64 local_file_size = com_file_size(file_path);
    if(local_file_size < 0)
    {
        local_file_size = 0;
    }
    if(local_file_size > remote_file_size)
    {
        LOG_W("file exist");
        return -101;
    }
    if(local_file_size == remote_file_size)
    {
        LOG_W("download may already finished");
        return local_file_size;
    }
#endif
    FILE* file = com_file_open(file_path, "wb+");
    if(file == NULL)
    {
        LOG_E("failed to create file %s", file_path);
        return -105;
    }

    CURL* curl = curl_easy_init();
    if(curl == NULL)
    {
        com_file_close(file);
        return -103;
    }

    struct curl_slist* curl_headers = NULL;
    for(size_t i = 0; i < headers.size() ; i++)
    {
        curl_headers = curl_slist_append(curl_headers, headers[i].c_str());
    }
    headers.clear();
#if 0
    if(local_file_size > 0)
    {
        LOG_W("file already exist, will resume the downlaod progress,local_file_size=%lld,file_path=%s", local_file_size, file_path);
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, local_file_size);
    }
    progress.setCurrentSizeBase(local_file_size);
#endif
    progress.setCurrentSizeBase(0);
    progress.setTotalSize(remote_file_size);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &progress);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, http_download_progress);
    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, connection_timeout_ms);
    curl_easy_setopt(curl, CURLOPT_POST, 0L);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if(timeout_ms > 0)
    {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
    }
    curl_easy_setopt(curl, CURLOPT_VERBOSE, debug_enable ? 1L : 0L); //打开调试日志
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_file_download_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    prepareCert(curl);

    int retry = retry_count;
    CURLcode ret = CURLE_OPERATION_TIMEOUTED;
    do
    {
        ret = curl_easy_perform(curl);
        if(ret != CURLE_OPERATION_TIMEOUTED)
        {
            break;
        }
        com_sleep_s(retry_count - retry);
    }
    while(retry--);

    if(curl_headers != NULL)
    {
        curl_slist_free_all(curl_headers);
    }

    if(ret != CURLE_OK)
    {
        LOG_E("download failed, ret=%d:%s,url=%s", ret, curl_easy_strerror(ret), url);
        curl_easy_cleanup(curl);
        com_file_close(file);
        return (-1 * ret);
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    if(code < 200 || code >= 300)
    {
        curl_easy_cleanup(curl);
        com_file_close(file);
        LOG_D("download failed,code=%ld,url=%s", code, url);
        return (-1 * code);
    }

    curl_easy_cleanup(curl);
    com_file_flush(file);
    com_file_close(file);
    return (int)com_file_size(file_path);
}

std::string ComexCurl::ConvertErrMessage(int err_code)
{
    const char* msg = curl_easy_strerror((CURLcode)err_code);
    if(msg == NULL)
    {
        return std::string();
    }
    return std::string(msg);
}

