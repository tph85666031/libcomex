#include "com_log.h"
#include "comex_poco.h"

#include <Poco/NotificationQueue.h>
#include <Poco/Notification.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/SecureServerSocket.h>
#include <Poco/Net/SecureStreamSocket.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/Context.h>
#include <Poco/Net/SSLException.h>
#include <Poco/Net/SSLManager.h>

using namespace Poco;
using namespace Poco::Net;

class MyHTTPRequestHandler : public HTTPRequestHandler
{
public:
    MyHTTPRequestHandler(ComexPocoProxyServer& server): server(server) {};
    template <class T>
    void handleHTTP(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        T session("www.baidu.com");
        //T session("172.21.23.72");
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
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        LOG_I("addr=%s", request.clientAddress().toString().c_str());
        handleHTTP<HTTPSClientSession>(request, response);
        //handleHTTP<HTTPClientSession>(request, response);
    }
private:
    ComexPocoProxyServer& server;
};

class MyHTTPRequestHandlerFactory : public HTTPRequestHandlerFactory
{
public:
    MyHTTPRequestHandlerFactory(ComexPocoProxyServer& server): server(server) {};
    Poco::Net::HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request)
    {
        return new MyHTTPRequestHandler(server);
    }
private:
    ComexPocoProxyServer& server;
};

template<class T>
class MyTCPServerConnection: public TCPServerConnection
{
public:
    MyTCPServerConnection(ComexPocoProxyServer& server, const StreamSocket& socket) : TCPServerConnection(socket), server(server) {};
    void run()
    {
        StreamSocket& socket_client = socket();
        char buf[4096];
        int ret = socket_client.receiveBytes(buf, sizeof(buf));
        LOG_I("ret=%d:%s", ret,buf);

        T socket_target;
        socket_target.connect(SocketAddress("www.baidu.com", 80));
        std::string request = "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n";
        socket_target.sendBytes(request.data(), request.size());
    }
private:
    ComexPocoProxyServer& server;
};

class MyTCPServerConnectionFactory : public TCPServerConnectionFactory
{
public:
    MyTCPServerConnectionFactory(ComexPocoProxyServer& server, bool with_ssl = false): server(server), with_ssl(with_ssl) {};
    TCPServerConnection* createConnection(const StreamSocket& socket)
    {
        if(with_ssl)
        {
            return new MyTCPServerConnection<SecureStreamSocket>(server, socket);
        }
        else
        {
            return new MyTCPServerConnection<StreamSocket>(server, socket);
        }
    }
private:
    ComexPocoProxyServer& server;
    bool with_ssl = false;
};

ComexPocoProxyServer::ComexPocoProxyServer()
{
}

ComexPocoProxyServer::~ComexPocoProxyServer()
{
    stopServer();
}

ComexPocoProxyServer& ComexPocoProxyServer::setPort(uint16 port_http, uint16 port_https, uint16 port_tcp, uint16 port_tcps)
{
    this->server_port_http = port_http;
    this->server_port_https = port_https;
    this->server_port_tcp = port_tcp;
    this->server_port_tcps = port_tcps;
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
    SSLManager::instance().initializeServer(NULL, NULL, Context::Ptr(new Context(Poco::Net::Context::TLS_SERVER_USE, ca_key, ca_crt, "", Poco::Net::Context::VERIFY_RELAXED, 9, false)));

    ServerSocket socket_http(server_port_http);
    SecureServerSocket socket_https(server_port_https);
    ServerSocket socket_tcp(server_port_tcp);
    SecureServerSocket socket_tcps(server_port_tcps);

    HTTPServerParams::Ptr params_http = new HTTPServerParams();
    params_http->setKeepAlive(true);
    params_http->setKeepAliveTimeout(Timespan(10, 0));
    params_http->setMaxThreads(16);

    TCPServerParams::Ptr params_tcp = new TCPServerParams();
    params_tcp->setMaxThreads(16);

    http_server = new HTTPServer(new MyHTTPRequestHandlerFactory(*this), socket_http, params_http);
    ((Poco::Net::HTTPServer*)http_server)->start();

    https_server = new HTTPServer(new MyHTTPRequestHandlerFactory(*this), socket_https, params_http);
    ((Poco::Net::HTTPServer*)https_server)->start();

    tcp_server = new TCPServer(new MyTCPServerConnectionFactory(*this), socket_tcp, params_tcp);
    ((Poco::Net::TCPServer*)tcp_server)->start();

    tcps_server = new TCPServer(new MyTCPServerConnectionFactory(*this, true), socket_tcps, params_tcp);
    ((Poco::Net::TCPServer*)tcps_server)->start();

    return true;
}

void ComexPocoProxyServer::stopServer()
{
    if(http_server != NULL)
    {
        ((Poco::Net::HTTPServer*)http_server)->stop();
        delete((Poco::Net::HTTPServer*)http_server);
        http_server = NULL;
    }
    if(https_server != NULL)
    {
        ((Poco::Net::HTTPServer*)https_server)->stop();
        delete((Poco::Net::HTTPServer*)https_server);
        https_server = NULL;
    }
    if(tcp_server != NULL)
    {
        ((Poco::Net::TCPServer*)tcp_server)->stop();
        delete((Poco::Net::TCPServer*)tcp_server);
        tcp_server = NULL;
    }
    if(tcps_server != NULL)
    {
        ((Poco::Net::TCPServer*)tcps_server)->stop();
        delete((Poco::Net::TCPServer*)tcps_server);
        tcps_server = NULL;
    }
    Poco::Net::SSLManager::instance().shutdown();
}

