#include <cassert>
#include "uv.h"
#include "comex_socket.h"
#include "com_log.h"

typedef struct
{
    uv_async_t async;
    uint32 val[4];
    void* ptr[4];
} uv_async_ext_t;

typedef struct
{
    uv_work_t work;
    uint32 val[4];
    void* ptr[4];
} uv_work_ext_t;

ComexTcpClient::ComexTcpClient()
{
}

ComexTcpClient::~ComexTcpClient()
{
    stopClient();
}

std::string ComexTcpClient::getHost()
{
    return host;
}

uint16 ComexTcpClient::getPort()
{
    return port;
}

void ComexTcpClient::setHost(const char* host, bool ipv6_first)
{
    if(host != NULL)
    {
        this->host = host;
    }
    this->host_ipv6_first = ipv6_first;
}

void ComexTcpClient::setPort(uint16 port)
{
    this->port = port;
}

void ComexTcpClient::reconnect()
{
    uv_timer_t* handle = new uv_timer_t();
    handle->data = this;
    uv_timer_init(uv_default_loop(), handle);
    uv_timer_start(handle, [](uv_timer_t* handle)
    {
        assert(handle != NULL);
        assert(handle->data != NULL);
        ComexTcpClient* ctx = (ComexTcpClient*)handle->data;
        ctx->connect();

        uv_close((uv_handle_t*)handle, [](uv_handle_t* handle)
        {
            delete handle;
        });
    }, 1000, false);
}

bool ComexTcpClient::connect()
{
    LOG_I("called");
    addrinfo hints;
    addrinfo* res = NULL;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = host_ipv6_first ? AF_INET6 : AF_INET;
    if(getaddrinfo(host.c_str(), NULL, &hints, &res) != 0)
    {
        LOG_E("failed to get host ip");
        return false;
    }
    if(res->ai_family == AF_INET)
    {
        struct sockaddr_in* addr_ipv4 = (struct sockaddr_in*)res->ai_addr;
        uv_ip4_addr(host.c_str(), port, addr_ipv4);
    }
    else if(res->ai_family == AF_INET6)
    {
        struct sockaddr_in6* addr_ipv6 = (struct sockaddr_in6*)res->ai_addr;
        uv_ip6_addr(host.c_str(), port, addr_ipv6);
    }
    else
    {
        freeaddrinfo(res);
        return false;
    }

    uv_connect_t* request = new uv_connect_t();
    ((uv_tcp_t*)handle_tcp)->data = this;
    uv_tcp_init(uv_default_loop(), (uv_tcp_t*)handle_tcp);
    uv_tcp_nodelay((uv_tcp_t*)handle_tcp, 1);
    int ret = uv_tcp_connect((uv_connect_t*)request, (uv_tcp_t*)handle_tcp, res->ai_addr,
                             [](uv_connect_t* request, int status)->void
    {
        assert(request != NULL);
        assert(request->handle != NULL);
        assert(request->handle->data != NULL);

        ComexTcpClient* ctx = (ComexTcpClient*)request->handle->data;
        ctx->onConnectionChanged(status == 0);

        if(status != 0)
        {
            LOG_I("connect failed,err=%s", uv_err_name(status));
            uv_close((uv_handle_t*)request->handle, [](uv_handle_t* handle)
            {
                ComexTcpClient* ctx = (ComexTcpClient*)handle->data;
                ctx->onConnectionChanged(false);
                ctx->reconnect();
            });
            delete request;
            return;
        }

        auto func_alloc = [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
        {
            assert(buf != NULL);
            buf->base = new char[suggested_size];
            buf->len = suggested_size;
        };

        auto func_read = [](uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
        {
            assert(handle != NULL);
            assert(handle->data != NULL);
            ComexTcpClient* ctx = (ComexTcpClient*)handle->data;

            if(nread > 0)
            {
                if(buf != NULL && buf->base != NULL)
                {
                    ctx->onRecv((const uint8*)buf->base, (int)nread);
                }
            }
            else if(nread < 0)
            {
                if(nread == UV_EOF)
                {
                    LOG_W("connection closed");
                }
                LOG_E("handle error:%s", uv_err_name(nread));
                uv_close((uv_handle_t*)handle, [](uv_handle_t* handle)
                {
                    ComexTcpClient* ctx = (ComexTcpClient*)handle->data;
                    ctx->onConnectionChanged(false);
                    ctx->reconnect();
                });
            }
            if(buf != NULL && buf->base != NULL)
            {
                delete[] buf->base;
            }
        };
        uv_read_start(request->handle, func_alloc, func_read);

        delete request;
    });
    freeaddrinfo(res);

    if(ret != 0)
    {
        LOG_E("connect failed,err=%s:%s", uv_err_name(ret), uv_strerror(ret));
        return false;
    }
    return true;
}

bool ComexTcpClient::startClient()
{
    handle_tcp = new uv_tcp_t();
    thread_loop = std::thread([&]()
    {
        LOG_I("loop start");
        connect();
        uv_run(uv_default_loop(), UV_RUN_DEFAULT);
        int ret = uv_loop_close(uv_default_loop());
        LOG_I("loop quit,ret=%d", ret);
    });

    return true;
}

void ComexTcpClient::stopClient()
{
    uv_async_t* handle = new uv_async_t();
    uv_async_init(uv_default_loop(), handle, [](uv_async_t* handle)
    {
        int ret = uv_loop_close(uv_default_loop());
        if(ret == UV_EBUSY)
        {
            uv_walk(uv_default_loop(), [](uv_handle_t* handle, void* arg)
            {
                uv_close(handle, [](uv_handle_t* handle)
                {
                    if(handle != NULL)
                    {
                        delete handle;
                    }
                });
            }, NULL);
        }
    });
    uv_async_send(handle);

    if(thread_loop.joinable())
    {
        thread_loop.join();
    }
}

int ComexTcpClient::sendData(const void* data, int data_size)
{
    if(data == NULL || data_size <= 0 || handle_tcp == NULL)
    {
        return -1;
    }
    uv_async_ext_t* handle_ext = new uv_async_ext_t();
    handle_ext->ptr[0] = (void*)new char[data_size];
    handle_ext->ptr[1] = handle_tcp;
    handle_ext->val[0] = data_size;
    memcpy(handle_ext->ptr[0], data, data_size);
    int ret = uv_async_init(uv_default_loop(), &handle_ext->async, [](uv_async_t* handle)
    {
        uv_async_ext_t* handle_ext = (uv_async_ext_t*)handle;
        auto func = [](uv_write_t* request, int status)
        {
            assert(request != NULL);
            assert(request->data != NULL);
            static int c = 0;
            if(status != 0)
            {
                LOG_E("write ret=%d,c=%d", status, ++c);
            }
            else
            {
                //LOG_I("write ret=%d,c=%d", status, ++c);
            }

            delete[](char*)request->data;
            delete request;
        };

        uv_buf_t uv_buf;
        uv_buf.base = (char*)handle_ext->ptr[0];
        uv_buf.len = handle_ext->val[0];
        uv_write_t* request = new uv_write_t();
        request->data = handle_ext->ptr[0];
        int ret = uv_write(request, (uv_stream_t*)handle_ext->ptr[1], &uv_buf, 1, func);
        if(ret != 0)
        {
            delete[](char*)handle_ext->ptr[0];
            delete  request;
            LOG_E("uv_write failed,err=%s:%s", uv_err_name(ret), uv_strerror(ret));
        }

        uv_close((uv_handle_t*)handle, [](uv_handle_t* handle)
        {
            delete handle;
        });
    });
    if(ret != 0)
    {
        delete[](char*)handle_ext->ptr[0];
        delete handle_ext;
        LOG_E("async send failed %s:%s", uv_err_name(ret), uv_strerror(ret));
        return -2;
    }
    uv_async_send(&handle_ext->async);
    return 0;
}

void ComexTcpClient::onConnectionChanged(bool connected)
{
    LOG_I("connected: %s", connected ? "yes" : "no");
}

void ComexTcpClient::onRecv(const uint8* data, int data_size)
{
    LOG_I("got message");
}

ComexTcpServer::ComexTcpServer()
{
    bind_ip = "0.0.0.0";
    handle_tcp_server = NULL;
    port = 0;
}

ComexTcpServer::~ComexTcpServer()
{
    stopServer();
}

uint16 ComexTcpServer::getPort()
{
    return port;
}

void ComexTcpServer::setPort(uint16 port)
{
    this->port = port;
}

void ComexTcpServer::setBindIP(const char* ip)
{
    if(ip != NULL)
    {
        this->bind_ip = ip;
    }
}

ComexSocketAddress ComexTcpServer::getClientAddress(void* handle)
{
    if(handle == NULL)
    {
        LOG_E("arg incorrect");
        return ComexSocketAddress();
    }

    struct sockaddr sock_addr;
    int sock_addr_len = sizeof(struct sockaddr);
    if(uv_tcp_getpeername((const uv_tcp_t*)handle, &sock_addr, &sock_addr_len) != 0)
    {
        LOG_E("failed");
        return ComexSocketAddress();
    }

    ComexSocketAddress addr;
    addr.family = sock_addr.sa_family;

    if(sock_addr.sa_family == AF_INET)
    {
        struct sockaddr_in* addr_ipv4 = (struct sockaddr_in*)&sock_addr;
        addr.ipv4 = addr_ipv4->sin_addr.s_addr;
        addr.port = ntohs(addr_ipv4->sin_port);
    }
    else if(sock_addr.sa_family == AF_INET6)
    {
        struct sockaddr_in6* addr_ipv6 = (struct sockaddr_in6*)&sock_addr;
        memcpy(addr.ipv6, addr_ipv6->sin6_addr.s6_addr, sizeof(addr.ipv6));
        addr.port = ntohs(addr_ipv6->sin6_port);
    }
    else
    {
        LOG_E("type not support:%d,sock_addr_len=%d", sock_addr.sa_family, sock_addr_len);
        return ComexSocketAddress();
    }
    return addr;
}

bool ComexTcpServer::startServer()
{
    handle_tcp_server = new uv_tcp_t();
    ((uv_tcp_t*)handle_tcp_server)->data = this;
    uv_tcp_init(uv_default_loop(), (uv_tcp_t*)handle_tcp_server);

    struct sockaddr_in addr;
    uv_ip4_addr(bind_ip.c_str(), port, &addr);

    uv_tcp_bind((uv_tcp_t*)handle_tcp_server, (const struct sockaddr*)&addr, 0);
    int ret = uv_listen((uv_stream_t*)handle_tcp_server, 128, [](uv_stream_t* server, int status)
    {
        if(status < 0)
        {
            LOG_E("failed status=%d:%s", status, uv_err_name(status));
            return;
        }

        uv_tcp_t* handle_client = new uv_tcp_t();
        handle_client->data = server->data;
        uv_tcp_init(uv_default_loop(), handle_client);
        int ret = uv_accept(server, (uv_stream_t*)handle_client);
        if(ret == 0)
        {
            ComexTcpServer* ctx = (ComexTcpServer*)handle_client->data;
            ctx->onConnectionChanged(handle_client, true);

            int ret = uv_read_start((uv_stream_t*)handle_client, [](uv_handle_t* handle_client, size_t suggested_size, uv_buf_t* buf)
            {
                assert(buf != NULL);
                buf->base = new char[suggested_size];
                buf->len = suggested_size;
            }, [](uv_stream_t* handle_client, ssize_t nread, const uv_buf_t* buf)
            {
                assert(buf != NULL);
                assert(handle_client != NULL);
                assert(handle_client->data != NULL);

                if(nread > 0 && buf != NULL && buf->base != NULL)
                {
                    //收到的数据放入工作队列中处理
                    uv_work_ext_t* request_work_ext = new uv_work_ext_t();
                    request_work_ext->ptr[0] = buf->base;
                    request_work_ext->val[0] = nread;
                    request_work_ext->ptr[1] = handle_client;
                    request_work_ext->work.data = handle_client->data;
                    uv_queue_work(uv_default_loop(), &request_work_ext->work, [](uv_work_t* request)
                    {
                        uv_work_ext_t* request_work_ext = (uv_work_ext_t*)request;
                        ComexTcpServer* ctx = (ComexTcpServer*)request->data;
                        ctx->onRecv(request_work_ext->ptr[1], (const uint8*)request_work_ext->ptr[0], request_work_ext->val[0]);
                        delete[](char*)request_work_ext->ptr[0];
                    }, [](uv_work_t* request, int status)
                    {
                        delete request;
                    });
                }
                else if(nread < 0)
                {
                    LOG_E("handle_client error:%s:%s", uv_err_name(nread), uv_strerror(nread));
                    uv_close((uv_handle_t*)handle_client, [](uv_handle_t* handle_client)
                    {
                        ComexTcpServer* ctx = (ComexTcpServer*)handle_client->data;
                        ctx->onConnectionChanged(handle_client, false);
                        delete handle_client;
                    });
                    if(buf != NULL && buf->base != NULL)
                    {
                        delete[] buf->base;
                    }
                }
                else
                {
                    LOG_E("failed");
                }
            });

            if(ret != 0)
            {
                LOG_E("start read failed,err=%s:%s", uv_err_name(ret), uv_strerror(ret));
                uv_close((uv_handle_t*)handle_client, [](uv_handle_t* handle)
                {
                    delete handle;
                });
            }
        }
        else
        {
            LOG_E("failed to accept,err=%s:%s", uv_err_name(ret), uv_strerror(ret));
            uv_close((uv_handle_t*)handle_client, [](uv_handle_t* handle)
            {
                delete handle;
            });
        }
    });

    if(ret != 0)
    {
        LOG_E("failed to listent");
        return false;
    }

    thread_loop = std::thread([&]()
    {
        LOG_I("loop start");
        uv_run(uv_default_loop(), UV_RUN_DEFAULT);
        int ret = uv_loop_close(uv_default_loop());
        LOG_I("loop quit,ret=%d", ret);
    });
    return true;
}

void ComexTcpServer::stopServer()
{
    LOG_I("caled");
    uv_async_t* handle = new uv_async_t();
    uv_async_init(uv_default_loop(), handle, [](uv_async_t* handle)
    {
        if(uv_loop_close(uv_default_loop()) == UV_EBUSY)
        {
            uv_walk(uv_default_loop(), [](uv_handle_t* handle, void* arg)
            {
                uv_close(handle, [](uv_handle_t* handle)
                {
                    if(handle != NULL)
                    {
                        delete handle;
                    }
                });
            }, NULL);
        }
    });
    uv_async_send(handle);

    if(thread_loop.joinable())
    {
        thread_loop.join();
    }
    LOG_I("done");
}

int ComexTcpServer::sendData(void* handle, const void* data, int data_size)
{
    if(handle == NULL || data == NULL || data_size <= 0)
    {
        return -1;
    }
    uv_async_ext_t* handle_ext = new uv_async_ext_t();
    handle_ext->ptr[0] = (void*)new char[data_size];
    handle_ext->ptr[1] = handle;
    handle_ext->val[0] = data_size;
    memcpy(handle_ext->ptr[0], data, data_size);
    uv_async_init(uv_default_loop(), &handle_ext->async, [](uv_async_t* handle)
    {
        uv_async_ext_t* handle_ext = (uv_async_ext_t*)handle;
        uv_write_t* request = new uv_write_t();
        request->data = handle_ext->ptr[0];
        auto func = [](uv_write_t* request, int status)
        {
            assert(request != NULL);
            assert(request->data != NULL);

            static int c = 0;
            LOG_I("write ret=%d,c=%d", status, ++c);

            delete[](char*)request->data;
            delete request;
        };

        uv_buf_t uv_buf;
        uv_buf.len = handle_ext->val[0];
        uv_buf.base = (char*)handle_ext->ptr[0];
        if(uv_write(request, (uv_stream_t*)handle_ext->ptr[1], &uv_buf, 1, func) != 0)
        {
            delete[] uv_buf.base;
            delete  request;
        }

        uv_close((uv_handle_t*)handle, [](uv_handle_t* handle)
        {
            delete handle;
        });
    });
    uv_async_send(&handle_ext->async);
    return 0;
}

void ComexTcpServer::onConnectionChanged(void* handle, bool connected)
{
    LOG_I("connection %p %s", handle, connected ? "connected" : "disconnected");
}

void ComexTcpServer::onRecv(void* handle, const uint8* data, int data_size)
{
    LOG_I("got message from %p", handle);
}

ComexPipeClient::ComexPipeClient()
{
}

ComexPipeClient::~ComexPipeClient()
{
    stopClient();
}

std::string ComexPipeClient::getServerName()
{
    return server_name;
}

void ComexPipeClient::setServerName(const char* name)
{
    if(name != NULL)
    {
        this->server_name = name;
    }
}

void ComexPipeClient::reconnect()
{
    LOG_I("called");
    uv_timer_t* handle = new uv_timer_t();
    handle->data = this;
    uv_timer_init(uv_default_loop(), handle);
    uv_timer_start(handle, [](uv_timer_t* handle)
    {
        assert(handle != NULL);
        assert(handle->data != NULL);
        ComexPipeClient* ctx = (ComexPipeClient*)handle->data;
        ctx->connect();

        uv_close((uv_handle_t*)handle, [](uv_handle_t* handle)
        {
            delete handle;
        });
    }, 1000, false);
}

bool ComexPipeClient::connect()
{
    LOG_I("called");
    uv_connect_t* request_conn = new uv_connect_t();
    ((uv_pipe_t*)handle_pipe)->data = this;
    uv_pipe_init(uv_default_loop(), (uv_pipe_t*)handle_pipe, 0);

#if 0
    int ret = uv_pipe_bind((uv_pipe_t*)handle_pipe, name.c_str());
    if(ret != 0)
    {
        LOG_E("bind failed,err=%s:%s", uv_err_name(ret), uv_strerror(ret));
        return false;
    }
#endif
    uv_pipe_connect((uv_connect_t*)request_conn, (uv_pipe_t*)handle_pipe, server_name.c_str(),
                    [](uv_connect_t* request, int status)->void
    {
        assert(request != NULL);
        assert(request->handle != NULL);
        assert(request->handle->data != NULL);
        ComexPipeClient* ctx = (ComexPipeClient*)request->handle->data;
        ctx->onConnectionChanged(status == 0);

        if(status != 0)
        {
            LOG_W("connect failed,err=%s:%s, will reconnect", uv_err_name(status), uv_strerror(status));
            uv_close((uv_handle_t*)request->handle, [](uv_handle_t* handle)
            {
                ComexPipeClient* ctx = (ComexPipeClient*)handle->data;
                ctx->onConnectionChanged(false);
                ctx->reconnect();
            });
            delete request;
            return;
        }

        auto func_alloc = [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
        {
            assert(buf != NULL);
            buf->base = new char[suggested_size];
            buf->len = suggested_size;
        };

        auto func_read = [](uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
        {
            assert(handle != NULL);
            assert(handle->data != NULL);
            ComexPipeClient* ctx = (ComexPipeClient*)handle->data;

            if(nread > 0)
            {
                if(buf != NULL && buf->base != NULL)
                {
                    ctx->onRecv((const uint8*)buf->base, (int)nread);
                }
            }
            else if(nread < 0)
            {
                if(nread == UV_EOF)
                {
                    LOG_W("connection closed");
                }
                LOG_E("handle error:%s:%s", uv_err_name(nread), uv_strerror(nread));
                uv_close((uv_handle_t*)handle, [](uv_handle_t* handle)
                {
                    ComexPipeClient* ctx = (ComexPipeClient*)handle->data;
                    ctx->onConnectionChanged(false);
                    ctx->reconnect();
                });
            }
            if(buf != NULL && buf->base != NULL)
            {
                delete[] buf->base;
            }
        };
        int ret = uv_read_start(request->handle, func_alloc, func_read);
        if(ret != 0)
        {
            LOG_E("read failed,err=%s:%s", uv_err_name(ret), uv_strerror(ret));
        }

        delete request;
    });
    return true;
}

bool ComexPipeClient::startClient()
{
    handle_pipe = new uv_pipe_t();
    if(connect() == false)
    {
        return false;
    }
    thread_loop = std::thread([&]()
    {
        LOG_I("loop start");
        uv_run(uv_default_loop(), UV_RUN_DEFAULT);
        int ret = uv_loop_close(uv_default_loop());
        LOG_I("loop quit,ret=%s", uv_err_name(ret));
    });

    return true;
}

void ComexPipeClient::stopClient()
{
    LOG_I("callled");
    uv_async_t* handle = new uv_async_t();
    uv_async_init(uv_default_loop(), handle, [](uv_async_t* handle)
    {
        if(uv_loop_close(uv_default_loop()) == UV_EBUSY)
        {
            uv_walk(uv_default_loop(), [](uv_handle_t* handle, void* arg)
            {
                uv_close(handle, [](uv_handle_t* handle)
                {
                    if(handle != NULL)
                    {
                        delete handle;
                    }
                });
            }, NULL);
        }
    });
    uv_async_send(handle);

    if(thread_loop.joinable())
    {
        thread_loop.join();
    }
}

int ComexPipeClient::sendData(const void* data, int data_size)
{
    if(data == NULL || data_size <= 0 || handle_pipe == NULL)
    {
        return -1;
    }
    uv_async_ext_t* handle_ext = new uv_async_ext_t();
    handle_ext->ptr[0] = (void*)new char[data_size];
    handle_ext->ptr[1] = handle_pipe;
    handle_ext->val[0] = data_size;
    memcpy(handle_ext->ptr[0], data, data_size);
    int ret = uv_async_init(uv_default_loop(), &handle_ext->async, [](uv_async_t* handle)
    {
        uv_async_ext_t* handle_ext = (uv_async_ext_t*)handle;
        uv_write_t* request = new uv_write_t();
        request->data = handle_ext->ptr[0];
        auto func = [](uv_write_t* request, int status)
        {
            assert(request != NULL);
            assert(request->data != NULL);
            static int c = 0;
            LOG_I("write ret=%d,c=%d", status, ++c);

            delete[](char*)request->data;
            delete request;
        };

        uv_buf_t uv_buf;
        uv_buf.len = handle_ext->val[0];
        uv_buf.base = (char*)handle_ext->ptr[0];
        int ret = uv_write(request, (uv_stream_t*)handle_ext->ptr[1], &uv_buf, 1, func);
        if(ret != 0)
        {
            delete[] uv_buf.base;
            delete  request;
            LOG_E("uv_write failed,err=%s:%s", uv_err_name(ret), uv_strerror(ret));
        }

        uv_close((uv_handle_t*)handle, [](uv_handle_t* handle)
        {
            delete handle;
        });
    });
    if(ret != 0)
    {
        delete[](char*)handle_ext->ptr[0];
        delete handle_ext;
        LOG_E("async send failed,err=%s:%s", uv_err_name(ret), uv_strerror(ret));
        return -2;
    }
    uv_async_send(&handle_ext->async);
    return 0;
}

void ComexPipeClient::onConnectionChanged(bool connected)
{
    LOG_I("connected: %s", connected ? "yes" : "no");
}

void ComexPipeClient::onRecv(const uint8* data, int data_size)
{
    LOG_I("got message");
}

ComexPipeServer::ComexPipeServer()
{
    handle_pipe_server = NULL;
}

ComexPipeServer::~ComexPipeServer()
{
    stopServer();
}

std::string ComexPipeServer::getName()
{
    return name;
}

void ComexPipeServer::setName(const char* name)
{
    if(name != NULL)
    {
        this->name = name;
    }
}

std::string ComexPipeServer::getClientName(void* handle)
{
    if(handle == NULL)
    {
        LOG_E("arg incorrect");
        return std::string();
    }

    char buf_addr[256];
    size_t buf_addr_size = sizeof(buf_addr);
    if(uv_pipe_getpeername((const uv_pipe_t*)handle, buf_addr, &buf_addr_size) != 0)
    {
        LOG_E("failed");
        return std::string();
    }
    buf_addr[buf_addr_size - 1] = '\0';
    return buf_addr;
}

bool ComexPipeServer::startServer()
{
    handle_pipe_server = new uv_pipe_t();
    ((uv_pipe_t*)handle_pipe_server)->data = this;
    uv_pipe_init(uv_default_loop(), (uv_pipe_t*)handle_pipe_server, 0);

    uv_fs_t request_fs;
    uv_fs_unlink(NULL, &request_fs, name.c_str(), NULL);
    int ret = uv_pipe_bind((uv_pipe_t*)handle_pipe_server, name.c_str());
    if(ret != 0)
    {
        LOG_E("bind failed,err=%s", uv_err_name(ret));
        return false;
    }
    ret = uv_listen((uv_stream_t*)handle_pipe_server, 128, [](uv_stream_t* server, int status)
    {
        if(status < 0)
        {
            LOG_E("failed status=%d:%s", status, uv_err_name(status));
            return;
        }

        uv_pipe_t* handle_client = new uv_pipe_t();
        handle_client->data = server->data;
        uv_pipe_init(uv_default_loop(), handle_client, 0);

        int ret = uv_accept(server, (uv_stream_t*)handle_client);
        if(ret == 0)
        {
            ComexPipeServer* ctx = (ComexPipeServer*)handle_client->data;
            ctx->onConnectionChanged(handle_client, true);

            ret = uv_read_start((uv_stream_t*)handle_client, [](uv_handle_t* handle_client, size_t suggested_size, uv_buf_t* buf)
            {
                if(buf == NULL)
                {
                    LOG_E("arg incorrect");
                    return;
                }
                buf->base = new char[suggested_size];
                buf->len = suggested_size;
            }, [](uv_stream_t* handle_client, ssize_t nread, const uv_buf_t* buf)
            {
                assert(handle_client != NULL);
                assert(handle_client->data != NULL);

                if(nread > 0 && buf != NULL && buf->base != NULL)
                {
                    //收到的数据放入工作队列中处理
                    uv_work_ext_t* request_work_ext = new uv_work_ext_t();
                    request_work_ext->ptr[0] = buf->base;
                    request_work_ext->val[0] = nread;
                    request_work_ext->ptr[1] = handle_client;
                    request_work_ext->work.data = handle_client->data;
                    uv_queue_work(uv_default_loop(), &request_work_ext->work, [](uv_work_t* request)
                    {
                        uv_work_ext_t* request_work_ext = (uv_work_ext_t*)request;
                        ComexPipeServer* ctx = (ComexPipeServer*)request->data;
                        ctx->onRecv(request_work_ext->ptr[1], (const uint8*)request_work_ext->ptr[0], request_work_ext->val[0]);
                        delete[](char*)request_work_ext->ptr[0];
                    }, [](uv_work_t* request, int status)
                    {
                        delete request;
                    });
                }
                else if(nread < 0)
                {
                    if(nread == UV_EOF)
                    {
                        LOG_W("connection closed");
                    }
                    LOG_E("handle_client error:%s", uv_err_name(nread));
                    uv_close((uv_handle_t*)handle_client, [](uv_handle_t* handle_client)
                    {
                        ComexPipeServer* ctx = (ComexPipeServer*)handle_client->data;
                        ctx->onConnectionChanged(handle_client, false);
                        delete handle_client;
                    });
                    if(buf != NULL && buf->base != NULL)
                    {
                        delete[] buf->base;
                    }
                }
            });

            if(ret != 0)
            {
                uv_close((uv_handle_t*)handle_client, [](uv_handle_t* handle)
                {
                    delete handle;
                });
            }
        }
        else
        {
            LOG_E("failed to accept,err=%s", uv_err_name(ret));
            uv_close((uv_handle_t*)handle_client, [](uv_handle_t* handle)
            {
                delete handle;
            });
        }
    });

    if(ret != 0)
    {
        LOG_E("failed to listent,err=%s", uv_err_name(ret));
        return false;
    }

    thread_loop = std::thread([&]()
    {
        LOG_I("loop start");
        uv_run(uv_default_loop(), UV_RUN_DEFAULT);
        int ret = uv_loop_close(uv_default_loop());
        LOG_I("loop quit,ret=%d", ret);
    });
    return true;
}

void ComexPipeServer::stopServer()
{
    uv_async_t* handle = new uv_async_t();
    uv_async_init(uv_default_loop(), handle, [](uv_async_t* handle)
    {
        if(uv_loop_close(uv_default_loop()) == UV_EBUSY)
        {
            uv_walk(uv_default_loop(), [](uv_handle_t* handle, void* arg)
            {
                uv_close(handle, [](uv_handle_t* handle)
                {
                    if(handle != NULL)
                    {
                        delete handle;
                    }
                });
            }, NULL);
        }
    });
    uv_async_send(handle);

    if(thread_loop.joinable())
    {
        thread_loop.join();
    }

    uv_fs_t request_fs;
    uv_fs_unlink(NULL, &request_fs, name.c_str(), NULL);
}

int ComexPipeServer::sendData(void* handle, const void* data, int data_size)
{
    if(handle == NULL || data == NULL || data_size <= 0)
    {
        return -1;
    }
    uv_async_ext_t* handle_ext = new uv_async_ext_t();
    handle_ext->ptr[0] = (void*)new char[data_size];
    handle_ext->ptr[1] = handle;
    handle_ext->val[0] = data_size;
    memcpy(handle_ext->ptr[0], data, data_size);
    uv_async_init(uv_default_loop(), &handle_ext->async, [](uv_async_t* handle)
    {
        uv_async_ext_t* handle_ext = (uv_async_ext_t*)handle;
        uv_write_t* request = new uv_write_t();
        request->data = handle_ext->ptr[0];
        auto func = [](uv_write_t* request, int status)
        {
            assert(request != NULL);
            assert(request->data != NULL);
            static int c = 0;
            LOG_I("write ret=%d,c=%d", status, ++c);

            delete[](char*)request->data;
            delete request;
        };

        uv_buf_t uv_buf;
        uv_buf.len = handle_ext->val[0];
        uv_buf.base = (char*)handle_ext->ptr[0];
        if(uv_write(request, (uv_stream_t*)handle_ext->ptr[1], &uv_buf, 1, func) != 0)
        {
            delete[] uv_buf.base;
            delete  request;
        }

        uv_close((uv_handle_t*)handle, [](uv_handle_t* handle)
        {
            delete handle;
        });
    });
    uv_async_send(&handle_ext->async);
    return 0;
}

void ComexPipeServer::onConnectionChanged(void* handle, bool connected)
{
    LOG_I("connection %p %s", handle, connected ? "connected" : "disconnected");
}

void ComexPipeServer::onRecv(void* handle, const uint8* data, int data_size)
{
    LOG_I("got message from %p", handle);
}

