#ifndef __COMEX_POCO_H__
#define __COMEX_POCO_H__

#include "com_base.h"
#include "com_thread.h"

#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/Context.h>
#include <Poco/Net/SSLException.h>

class ComexPocoProxyServer
{
public:
    ComexPocoProxyServer();
    virtual ~ComexPocoProxyServer();

    ComexPocoProxyServer& setPort(uint16 port);
    ComexPocoProxyServer& setCA(const char* ca_crt, const char* ca_key);
    bool startServer();
    void stopServer();
private:
    uint16 proxy_server_port;
    std::string ca_crt;
    std::string ca_key;

    Poco::Net::HTTPServer* http_server = NULL ;
};

#endif /* __COMEX_POCO_H__ */
