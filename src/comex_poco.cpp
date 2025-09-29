#include "com_log.h"
#include "comex_poco.h"

#include <Poco/NotificationQueue.h>
#include <Poco/Notification.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/SecureStreamSocket.h>
#include <Poco/Net/SSLException.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/SecureServerSocket.h>

using namespace Poco;
using namespace Poco::Net;

class ProxyHTTPRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        LOG_I("addr=%s", request.clientAddress().toString().c_str());
        HTTPSClientSession session("www.baidu.com");

        Poco::Net::HTTPRequest forward_request(request.getMethod(),
                                               request.getURI(),
                                               request.getVersion());
        LOG_I("method=%s,uri=%s", request.getMethod().c_str(), request.getURI().c_str());
        // 复制请求头
        for(auto it = request.begin(); it != request.end(); ++it)
        {
            forward_request.set(it->first, it->second);
            LOG_I("%s:%s", it->first.c_str(), it->second.c_str());
        }
        forward_request.erase("Host");

        // 发送请求
        std::istream& request_stream = request.stream();
        std::ostream& forward_stream = session.sendRequest(forward_request);
        char buf[4096];
        do
        {
            request_stream.read(buf, sizeof(buf));
            int read_size = request_stream.gcount();
            if(read_size <= 0)
            {
                break;
            }
            //TODO:dlp the buf
            forward_stream.write(buf, read_size);
        }
        while(true);

        // 获取响应
        Poco::Net::HTTPResponse forward_response;
        std::string responseBody;
        std::istream& forward_response_stream = session.receiveResponse(forward_response);

        // 转发响应给客户端
        response.setStatus(forward_response.getStatus());
        response.setReason(forward_response.getReason());

        // 复制响应头
        for(auto it = forward_response.begin(); it != forward_response.end(); ++it)
        {
            response.set(it->first, it->second);
        }

        std::ostream& client_stream = response.send();
        do
        {
            forward_response_stream.read(buf, sizeof(buf));
            int read_size = forward_response_stream.gcount();
            if(read_size <= 0)
            {
                break;
            }
            //TODO:dlp the buf
            client_stream.write(buf, read_size);
        }
        while(true);
    }
};

class ProxyHTTPRequestHandlerFactory : public HTTPRequestHandlerFactory
{
public:
    Poco::Net::HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request)
    {
        return new ProxyHTTPRequestHandler();
    }
};

ComexPocoProxyServer::ComexPocoProxyServer()
{
}

ComexPocoProxyServer::~ComexPocoProxyServer()
{
    stopServer();
}

ComexPocoProxyServer& ComexPocoProxyServer::setPort(uint16 port)
{
    this->proxy_server_port = port;
    return *this;
}

ComexPocoProxyServer& ComexPocoProxyServer::setCA(const char* ca_crt, const char* ca_key)
{
    if(ca_crt != NULL)
    {
        this->ca_crt = ca_crt;
    }
    if(ca_key != NULL)
    {
        this->ca_key = ca_key;
    }
    return *this;
}

bool ComexPocoProxyServer::startServer()
{
    SSLManager::instance().initializeServer(NULL, NULL, Context::Ptr(new Context(
            Poco::Net::Context::TLS_SERVER_USE,
            ca_key,   // 根证书私钥
            ca_crt,  // 根证书
            "",            // 无CA链
            Poco::Net::Context::VERIFY_RELAXED,
            9,
            false)));
    SecureServerSocket socket(proxy_server_port);
    HTTPServerParams* params = new HTTPServerParams();
    params->setKeepAlive(true);
    params->setKeepAliveTimeout(Timespan(10, 0));
    params->setMaxThreads(16);

    http_server = new HTTPServer(new ProxyHTTPRequestHandlerFactory(), socket, params);
    http_server->start();
    return true;
}

void ComexPocoProxyServer::stopServer()
{
    if(http_server != NULL)
    {
        http_server->stop();
        delete http_server;
        http_server = NULL;
    }
    Poco::Net::SSLManager::instance().shutdown();
}
