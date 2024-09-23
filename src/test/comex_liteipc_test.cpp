#include "comex.h"
#include "com_log.h"
#include "com_test.h"

class MyLiteIPC : public LiteIPC
{
public:
    MyLiteIPC() {};
    ~MyLiteIPC() {};

    virtual ComBytes onRecvControl(uint32 addr, uint32 id, uint8* data, int data_size)
    {
        count_control++;
        LOG_I("got control from %u,msg=%s,data_size=%d,count=%d", addr, (char*)data, data_size, count_control.load());
        if(count_control == count_control_max)
        {
            sem.post();
        }
        return bytes;
    }

    virtual void onRecvStatus(uint32 addr, uint32 id, uint8* data, int data_size)
    {
        count_status++;
        LOG_I("got status from %u,msg=%s", addr, (char*)data);
    }

    virtual void onRecvEvent(uint32 addr, uint32 id, uint8* data, int data_size)
    {
        count_event++;
        LOG_I("got event from %u,msg=%s", addr, (char*)data);
    }

    ComBytes bytes;
    std::atomic<int> count_control = {0};
    std::atomic<int> count_control_max = {0};
    std::atomic<int> count_event = {0};
    std::atomic<int> count_status = {0};

    ComSem sem;
};

void th_test(MyLiteIPC* ipc)
{
    for(int i = 0; i < ipc->count_control; i++)
    {
        ipc->sendControl(2, 1, ipc->bytes.getData(), ipc->bytes.getDataSize());
    }
}

void comex_liteipc_unit_test_suit(void** state)
{
    TIME_COST();

    uint8 buf[50 * 1024];
    ComBytes send_data(sizeof(buf) * 5);
    ComBytes reply_data(sizeof(buf) * 5);

    TIME_COST_SHOW();
    for(int i = 0; i < 1; i++)
    {
        send_data.append(buf, sizeof(buf));
        reply_data.append(buf, sizeof(buf));
    }
    TIME_COST_SHOW();

    comex_mqtt_global_init();
    //com_log_set_level("DEBUG");
    MyLiteIPC ipc_a;
    MyLiteIPC ipc_b;
    ipc_a.setHost("127.0.0.1").setPort(1883);
    ipc_a.setAddr(1);
    ipc_a.bytes = send_data;
    ipc_a.startIPC();

    ipc_b.setHost("127.0.0.1").setPort(1883);
    ipc_b.setAddr(2);
    ipc_b.bytes = reply_data;
    ipc_b.addStatusListener(1);
    ipc_b.addEventListener(1);
    ipc_b.startIPC();

    int count_max = 100;
    int thread_count = 100;
    ipc_a.count_control = count_max;
    ipc_b.count_control_max = count_max * thread_count;
    for(int i = 0; i < thread_count; i++)
    {
        std::thread t = std::thread(th_test, &ipc_a);
        t.detach();
    }
    for(int i = 0; i < 1; i++)
    {
        ipc_a.sendStatus(1, "status1", sizeof("status1"));
        ipc_a.sendEvent(1, "event1", sizeof("event1"));
    }
    ipc_b.sem.wait(30 * 1000);
    TIME_COST_SHOW();
    LOG_I("control count=%d,event count=%d,status count=%d", ipc_b.count_control.load(), ipc_b.count_event.load(), ipc_b.count_status.load());
    comex_mqtt_global_uninit();
    ASSERT_TRUE(ipc_b.count_control >= ipc_b.count_control_max);
}

