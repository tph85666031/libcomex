#ifndef __COMEX_SOCKET_H__
#define __COMEX_SOCKET_H__

#include "com_base.h"

class COM_EXPORT ComexSocketAddress
{
public:
    bool valid = false;
    int family = 0;
    uint32 ipv4 = 0;
    uint8 ipv6[16] = {0};
    uint16 port = 0;
};

class COM_EXPORT ComexTcpClient
{
public:
    ComexTcpClient();
    virtual ~ComexTcpClient();

    std::string getHost();
    uint16 getPort();

    void setHost(const char* host, bool ipv6_first = false);
    void setPort(uint16 port);

    bool startClient();
    void stopClient();

    int sendData(const void* data, int data_size);
    void reconnect();
private:
    bool connect();
protected:
    virtual void onRecv(const uint8* data, int data_size);
    virtual void onConnectionChanged(bool connected);
private:
    std::string host;
    bool host_ipv6_first;
    uint16 port;

    void* handle_tcp;
    std::thread thread_loop;
};

class COM_EXPORT ComexTcpServer
{
public:
    ComexTcpServer();
    virtual ~ComexTcpServer();

    uint16 getPort();
    void setPort(uint16 port);
    void setBindIP(const char* ip);
    ComexSocketAddress getClientAddress(void* handle);

    bool startServer();
    void stopServer();
    int sendData(void* handle, const void* data, int data_size);

protected:
    virtual void onConnectionChanged(void* handle, bool connected);
    virtual void onRecv(void* handle, const uint8* data, int data_size);

private:
    uint16 port;
    std::string bind_ip;
    void* handle_tcp_server;
    void* handle_idle;
    std::thread thread_loop;
};

class COM_EXPORT ComexPipeClient
{
public:
    ComexPipeClient();
    virtual ~ComexPipeClient();

    std::string getServerName();
    void setServerName(const char* name);

    bool startClient();
    void stopClient();

    int sendData(const void* data, int data_size);
    void reconnect();
private:
    bool connect();
protected:
    virtual void onRecv(const uint8* data, int data_size);
    virtual void onConnectionChanged(bool connected);
private:
    std::string server_name;
    void* handle_pipe;
    std::thread thread_loop;
};

class COM_EXPORT ComexPipeServer
{
public:
    ComexPipeServer();
    virtual ~ComexPipeServer();

    std::string getName();
    void setName(const char* name);
    std::string getClientName(void* handle);

    bool startServer();
    void stopServer();
    int sendData(void* handle, const void* data, int data_size);

protected:
    virtual void onConnectionChanged(void* handle, bool connected);
    virtual void onRecv(void* handle, const uint8* data, int data_size);

private:
    std::string name;
    void* handle_pipe_server;
    std::thread thread_loop;
};

#endif /* __COMEX_SOCKET_H__ */

