#ifndef __COMEX_LLTEIPC_H__
#define __COMEX_LLTEIPC_H__

#include "comex_mqtt.h"

#define LITEIPC_ADDR_ALL    0xFFFFFFFF
#define LITEIPC_ID_ALL      0xFFFFFFFF

class COM_EXPORT LiteIPC : public MqttClient
{
public:
    LiteIPC();
    LiteIPC(uint32 addr);
    virtual ~LiteIPC();

    uint32 getAddr();
    LiteIPC& setAddr(uint32 addr);
    LiteIPC& setWill(uint32 id, const void* data, int data_size);

    bool startIPC();
    void stopIPC();
    
    bool addStatusListener(uint32 addr, uint32 id = LITEIPC_ID_ALL);
    bool addEventListener(uint32 addr, uint32 id = LITEIPC_ID_ALL);
    void removeStatusListener(uint32 addr, uint32 id = LITEIPC_ID_ALL);
    void removeEventListener(uint32 addr, uint32 id = LITEIPC_ID_ALL);

    ComBytes sendControl(uint32 addr, uint32 id, const void* data, int data_size, int timeout_ms = 5000);
    bool sendStatus(uint32 id, const void* data, int data_size);
    bool sendEvent(uint32 id, const void* data, int data_size);
private:
    virtual ComBytes onRecvControl(uint32 addr, uint32 id, uint8* data, int data_size)
    {
        return ComBytes();
    };
    virtual void onRecvStatus(uint32 addr, uint32 id, uint8* data, int data_size) {};
    virtual void onRecvEvent(uint32 addr, uint32 id, uint8* data, int data_size) {};
    void onRecv(const std::string& topic, const uint8* data, int data_size, MqttProperty& prop);
    static void ThreadRx(LiteIPC* ctx);
private:
    uint32 addr = 0;
    std::string name;

    int qos_control = MQTT_QOS2;
    int qos_status = MQTT_QOS1;
    int qos_event = MQTT_QOS0;

    std::atomic<uint32> magic = {0};

    std::thread thread_rx;
    std::atomic<bool> thread_rx_running;

    std::mutex mutex_rx_queue;
    ComCondition condition_rx_queue;
    std::queue<ComBytes> rx_queue;
};

#endif /* __COMEX_LLTEIPC_H__ */

