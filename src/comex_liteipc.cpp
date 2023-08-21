#include <map>
#include "comex_liteipc.h"
#include "com_thread.h"
#include "com_log.h"
#include "com_sync.h"

#define LITE_IPC_FLAG_ACK         0x80
#define LITE_IPC_FLAG_CONTROL     0x01
#define LITE_IPC_FLAG_STATUS      0x02
#define LITE_IPC_FLAG_EVENT       0x04

//单机使用，暂不考虑字节序问题
#pragma pack(push)
#pragma pack(1)
typedef struct
{
    uint32 from;
    uint32 to;
    uint8 flag;
    uint32 magic;
    uint32 id;
    uint32 len;
    uint8 data[0];
} IPC_MSG;
#pragma pack(pop)

LiteIPC::LiteIPC()
{
}

LiteIPC::LiteIPC(uint32 addr)
{
    setAddr(addr);
}

LiteIPC::~LiteIPC()
{
    stopIPC();
}

uint32 LiteIPC::getAddr()
{
    return addr;
}

LiteIPC& LiteIPC::setAddr(uint32 addr)
{
    this->addr = addr;
    setClientID(std::to_string(addr).c_str());
    return *this;
}

LiteIPC& LiteIPC::setWill(uint32 id, const void* data, int data_size)
{
    if(data == NULL || data_size <= 0)
    {
        setWillInfo(NULL, 0, NULL);
    }
    else
    {
        IPC_MSG msg;
        msg.from = addr;
        msg.to = addr;
        msg.flag = LITE_IPC_FLAG_STATUS;
        msg.magic = magic++;
        msg.id = id;
        msg.len = data_size;
        CPPBytes bytes(sizeof(IPC_MSG) + data_size);
        bytes.append((uint8*)&msg, sizeof(IPC_MSG));
        bytes.append((const uint8*)data, data_size);
        std::string topic = com_string_format("/%u/OUT/STATUS/%u", addr, id);
        setWillInfo(bytes.getData(), bytes.getDataSize(), topic.c_str(), qos_status, true);
    }
    return *this;
}

bool LiteIPC::startIPC()
{
    thread_rx_running = true;
    thread_rx = std::thread(ThreadRx, this);

    if(startClient(true) == false)
    {
        return false;
    }

    std::string topic = com_string_format("/%u/IN", addr);
    return subscribe(topic.c_str());
}

void LiteIPC::stopIPC()
{
    stopClient();
    thread_rx_running = false;
    if(thread_rx.joinable())
    {
        thread_rx.join();
    }
}

bool LiteIPC::addStatusListener(uint32 addr, uint32 id)
{
    //订阅指定设备的状态
    std::string topic = com_string_format("/%s/OUT/STATUS/%s", 
                        addr == LITEIPC_ADDR_ALL ? "+" : std::to_string(addr).c_str(),
                        id == LITEIPC_ID_ALL ? "+" : std::to_string(id).c_str());
    return subscribe(topic.c_str(), qos_status);
}

bool LiteIPC::addEventListener(uint32 addr, uint32 id)
{
    //订阅指定设备的状态
    std::string topic = com_string_format("/%s/OUT/EVENT/%s", 
                        addr == LITEIPC_ADDR_ALL ? "+" : std::to_string(addr).c_str(),
                        id == LITEIPC_ID_ALL ? "+" : std::to_string(id).c_str());
    return subscribe(topic.c_str(), qos_status);
}

void LiteIPC::removeStatusListener(uint32 addr, uint32 id)
{
    std::string topic = com_string_format("/%s/OUT/STATUS/%s", 
                        addr == LITEIPC_ADDR_ALL ? "+" : std::to_string(addr).c_str(),
                        id == LITEIPC_ID_ALL ? "+" : std::to_string(id).c_str());
    unsubscribe(topic.c_str());
    return;
}

void LiteIPC::removeEventListener(uint32 addr, uint32 id)
{
    std::string topic = com_string_format("/%s/OUT/EVENT/%s", 
                        addr == LITEIPC_ADDR_ALL ? "+" : std::to_string(addr).c_str(),
                        id == LITEIPC_ID_ALL ? "+" : std::to_string(id).c_str());
    unsubscribe(topic.c_str());
    return;
}

void LiteIPC::onRecv(const std::string& topic, const uint8* data, int data_size, MqttProperty& prop)
{
    if(data == NULL || data_size <= 0)
    {
        LOG_E("arg incorrect");
        return;
    }

    IPC_MSG* msg = (IPC_MSG*)data;

    if(msg->flag & LITE_IPC_FLAG_ACK)
    {
        GetSyncManager().getAdapter("comex_liteipc").syncPost(msg->magic, msg->data, msg->len);
    }
    else
    {
        mutex_rx_queue.lock();
        rx_queue.push(CPPBytes(data, data_size));
        mutex_rx_queue.unlock();
        condition_rx_queue.notifyAll();
    }
    return;
}

CPPBytes LiteIPC::sendControl(uint32 addr, uint32 id, const void* data, int data_size, int timeout_ms)
{
    if(data == NULL || data_size <= 0)
    {
        LOG_E("arg incorrect");
        return CPPBytes();
    }

    IPC_MSG msg;
    msg.from = this->addr;
    msg.to = addr;
    msg.flag = LITE_IPC_FLAG_CONTROL;
    msg.magic = magic++;
    msg.id = id;
    msg.len = data_size;

    CPPBytes bytes(sizeof(IPC_MSG) + data_size);
    bytes.append((uint8*)&msg, sizeof(IPC_MSG));
    bytes.append((uint8*)data, data_size);
    std::string topic = com_string_format("/%u/IN", addr);

    if(isConnected() == false || timeout_ms <= 0)
    {
        publish(topic.c_str(), bytes.getData(), bytes.getDataSize(), qos_control, false);
        return CPPBytes();
    }

    GetSyncManager().getAdapter("comex_liteipc").syncPrepare(msg.magic);
    if(publish(topic.c_str(), bytes.getData(), bytes.getDataSize(), qos_control, false) == false)
    {
        GetSyncManager().getAdapter("comex_liteipc").syncCancel(msg.magic);
        LOG_E("failed");
        return CPPBytes();
    }

    return GetSyncManager().getAdapter("comex_liteipc").syncWait(msg.magic, timeout_ms);
}

bool LiteIPC::sendStatus(uint32 id, const void* data, int data_size)
{
    if(data == NULL || data_size <= 0)
    {
        LOG_E("arg incorrect");
        return false;
    }

    IPC_MSG msg;
    msg.from = addr;
    msg.to = addr;
    msg.flag = LITE_IPC_FLAG_STATUS;
    msg.magic = magic++;
    msg.id = id;
    msg.len = data_size;

    CPPBytes bytes(sizeof(IPC_MSG) + data_size);
    bytes.append((uint8*)&msg, sizeof(IPC_MSG));
    bytes.append((uint8*)data, data_size);

    std::string topic = com_string_format("/%u/OUT/STATUS/%u", addr, id);
    return publish(topic.c_str(), bytes.getData(), bytes.getDataSize(), qos_status, true);
}

bool LiteIPC::sendEvent(uint32 id, const void* data, int data_size)
{
    if(data == NULL || data_size <= 0)
    {
        LOG_E("arg incorrect");
        return false;
    }

    IPC_MSG msg;
    msg.from = addr;
    msg.to = addr;
    msg.flag = LITE_IPC_FLAG_EVENT;
    msg.magic = magic++;
    msg.id = id;
    msg.len = data_size;

    CPPBytes bytes(sizeof(IPC_MSG) + data_size);
    bytes.append((uint8*)&msg, sizeof(IPC_MSG));
    bytes.append((uint8*)data, data_size);

    std::string topic = com_string_format("/%u/OUT/EVENT/%u", addr, id);
    return publish(topic.c_str(), bytes.getData(), bytes.getDataSize(), qos_event, false);
}

void LiteIPC::ThreadRx(LiteIPC* ctx)
{
    LOG_D("running...");
    if(ctx == NULL)
    {
        LOG_E("arg is NULL");
        return;
    }

    while(ctx->thread_rx_running)
    {
        ctx->condition_rx_queue.wait(1000);
        while(ctx->thread_rx_running)
        {
            ctx->mutex_rx_queue.lock();
            if(ctx->rx_queue.empty())
            {
                ctx->mutex_rx_queue.unlock();
                break;
            }
            CPPBytes bytes = std::move(ctx->rx_queue.front());
            ctx->rx_queue.pop();
            ctx->mutex_rx_queue.unlock();

            if(bytes.empty())
            {
                continue;
            }

            IPC_MSG* msg = (IPC_MSG*)bytes.getData();

            if(msg->flag == LITE_IPC_FLAG_CONTROL)
            {
                CPPBytes reply = std::move(ctx->onRecvControl(msg->from, msg->id, msg->data, msg->len));
                IPC_MSG msg_reply;
                msg_reply.from = ctx->addr;
                msg_reply.to = msg->from;
                msg_reply.flag = LITE_IPC_FLAG_CONTROL | LITE_IPC_FLAG_ACK;
                msg_reply.magic = msg->magic;
                msg_reply.id = msg->id;
                msg_reply.len = reply.getDataSize();

                reply.insert(0, (uint8*)&msg_reply, sizeof(IPC_MSG));
                std::string topic = com_string_format("/%u/IN", msg->from);
                ctx->publish(topic.c_str(), reply.getData(), reply.getDataSize(), ctx->qos_control, false);
            }
            else if(msg->flag == LITE_IPC_FLAG_STATUS)
            {
                ctx->onRecvStatus(msg->from, msg->id, msg->data, msg->len);
            }
            else if(msg->flag == LITE_IPC_FLAG_EVENT)
            {
                ctx->onRecvEvent(msg->from, msg->id, msg->data, msg->len);
            }
        }
    }
    LOG_D("quit");
    return;
}

