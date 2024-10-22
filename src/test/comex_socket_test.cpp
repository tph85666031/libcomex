#include "com_log.h"
#include "com_socket.h"
#include "comex_socket.h"

int64 count_per_client = 10000;
std::atomic<int64> client_tx_count = {0};
std::atomic<int64> client_rx_count = {0};
std::atomic<int64> server_tx_count = {0};
std::atomic<int64> server_rx_count = {0};

class MyComexTcpClient : public ComexTcpClient
{
public:
    MyComexTcpClient()
    {
    };
    ~MyComexTcpClient()
    {
    };

    void onRecv(const uint8* data, int data_size)
    {
        client_rx_count++;
        LOG_I("got data:%s", (const char*)data);
    }
    void onConnectionChanged(bool connected)
    {
        LOG_I("connected: %s", connected ? "yes" : "no");
        if(connected)
        {
            startTest();
        }
        LOG_I("send done");
    }

    void startTest()
    {
        thread_test = std::thread([&]()
        {
            for(int64 i = 0; i < count_per_client; i++)
            {
                //LOG_I("send %lld", i);
                std::string text = com_string_format("test %lld", i);
                if(sendData(text.c_str(), text.length() + 1) == 0)
                {
                    client_tx_count++;
                }
                else
                {
                    LOG_E("send failed:%lld", i);
                }
                //com_sleep_ms(1);
            }
        });
        thread_test.detach();
    }

    std::thread thread_test;
};

class MySocketTcpServer : public ComTcpServer
{
public:
    MySocketTcpServer()
    {
    }
    ~MySocketTcpServer()
    {
    }
    void onConnectionChanged(std::string& host, uint16 port, int socketfd, bool connected)
    {
    }

    void onRecv(std::string& host, uint16 port, int socketfd, uint8* data, int data_size)
    {
        server_rx_count++;
        LOG_I("got %lld:%s", server_rx_count.load(), (char*)data);
    }
};

class MyComexTcpServer : public ComexTcpServer
{
public:
    MyComexTcpServer() {};
    ~MyComexTcpServer() {};

    void onConnectionChanged(void* handle, bool connected)
    {
        ComexSocketAddress addr = getClientAddress(handle);
        LOG_I("client %s,handle=%p, %s:%u", connected ? "connected" : "disconnected", handle, com_ipv4_to_string(addr.ipv4).c_str(), addr.port);
    }
    void onRecv(void* handle, const uint8* data, int data_size)
    {
        server_rx_count++;
        LOG_I("got %lld:%s", server_rx_count.load(), (char*)data);
        //ComexSocketAddress addr = getClientAddress(handle);
        //LOG_I("got data from %p:%s,ip=%s:%u", handle, (const char*)data, com_ipv4_to_string(addr.ipv4).c_str(), addr.port);
        //if(sendData(handle, data, data_size) == 0)
        {
            //server_tx_count++;
        }
    }
};

class MyComexPipeClient : public ComexPipeClient
{
public:
    MyComexPipeClient()
    {
    };
    ~MyComexPipeClient()
    {
    };

    void onRecv(const uint8* data, int data_size)
    {
        LOG_I("got data:%s", (const char*)data);
        client_rx_count++;
    }
    void onConnectionChanged(bool connected)
    {
        LOG_I("connected: %s", connected ? "yes" : "no");
        if(connected)
        {
            startTest();
        }
    }

    void startTest()
    {
        thread_test = std::thread([&]()
        {
            for(int64 i = 0; i < count_per_client; i++)
            {
                LOG_I("send %lld", i);
                std::string text = com_string_format("test %lld", i);
                if(sendData(text.c_str(), text.length() + 1) == 0)
                {
                    client_tx_count++;
                }
                else
                {
                    LOG_E("send failed:%lld", i);
                }
                //com_sleep_ms(100);
            }
        });
        thread_test.detach();
    }

    std::thread thread_test;
};

class MyComexPipeServer : public ComexPipeServer
{
public:
    MyComexPipeServer() {};
    ~MyComexPipeServer() {};

    void onConnectionChanged(void* handle, bool connected)
    {
        std::string clinet_name = getClientName(handle);
        LOG_I("client %s,handle=%p,name=%s", connected ? "connected" : "disconnected", handle, clinet_name.c_str());
    }
    void onRecv(void* handle, const uint8* data, int data_size)
    {
        server_rx_count++;
        LOG_I("got %lld", server_rx_count.load());
        //std::string clinet_name = getClientName(handle);
        //LOG_I("got data from %p:%s,data=%s", handle, clinet_name.c_str(), (const char*)data);
        //if(sendData(handle, data, data_size) == 0)
        {
            //server_tx_count++;
        }
    }
};

void comex_socket_unit_test_suit(void** state)
{
    LOG_I("called");
    count_per_client = 1000;
    if(com_string_equal(getenv("TYPE"), "pipe"))
    {
        if(com_string_equal(getenv("NODE"), "client"))
        {
            MyComexPipeClient client;
            client.setServerName("/tmp/server.pipe");
            client.startClient();
            getchar();
        }
        else
        {
            MyComexPipeServer server;
            server.setName("/tmp/server.pipe");
            server.startServer();
            getchar();
        }
    }
    else
    {
        if(com_string_equal(getenv("NODE"), "client"))
        {
            MyComexTcpClient client;
            client.setHost("127.0.0.1");
            client.setPort(8888);
            client.startClient();
            LOG_I("start done");
            client.sendData("test udp message", sizeof("test udp message"));
            getchar();
        }
        else
        {
            MyComexTcpServer server;
            server.setPort(8888);
            server.startServer();
            getchar();
        }
    }
    LOG_I("test quit,client_tx=%lld,client_rx=%lld,server_tx=%lld,server_rx=%lld",
          client_tx_count.load(), client_rx_count.load(),
          server_tx_count.load(), server_rx_count.load());
}

