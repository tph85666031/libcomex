#ifndef __COMEX_POCO_H__
#define __COMEX_POCO_H__

#include "com_base.h"
#include "com_thread.h"

class ComexPocoProxyServer
{
public:
    ComexPocoProxyServer();
    virtual ~ComexPocoProxyServer();

    ComexPocoProxyServer& setPort(uint16 port_http, uint16 port_https, uint16 port_tcp, uint16 port_tcps);
    ComexPocoProxyServer& setCA(const char* ca_crt, const char* ca_key);
    bool startServer();
    void stopServer();
private:
    uint16 server_port_http;
    uint16 server_port_https;
    uint16 server_port_tcp;
    uint16 server_port_tcps;
    std::string ca_crt;
    std::string ca_key;

    void* http_server = NULL;
    void* https_server = NULL;
    void* tcp_server = NULL;
    void* tcps_server = NULL;
};

#endif /* __COMEX_POCO_H__ */
