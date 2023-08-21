#include "comex.h"
#include "com_log.h"
#include "com_test.h"

class MyMqttClient : public MqttClient
{
public:
	void onRecv(const std::string& topic, const uint8* data, int data_size, MqttProperty& prop)
    {
        LOG_I("[CB]GOT Message FROM MQTT,topic=%s, data=%s,count=%d", topic.c_str(), (char*)data, ++count);
    }

    void onConnectionChanged(bool connected, MqttProperty& prop)
    {
        if (connected)
        {
            LOG_I("connection succeed");
        }
        else
        {
            LOG_I("connection failed");
        }
    }
    int count = 0 ;
};

void comex_mqtt_unit_test_suit(void** state)
{
	comex_mqtt_global_init();
    //com_run_shell("sudo pkill mosquitto");
    //com_run_shell("mosquitto -d");
    MyMqttClient client;
    client.setHost("127.0.0.1").setPort(1883).setClientID("13550075599");
    ASSERT_TRUE(client.startClient(true, 1000));
    ASSERT_TRUE(client.subscribe("mqtt_test_topic"));

    com_sleep_s(1);

    for (int i = 0; i < 10; i++)
    {
        bool ret = client.publish("mqtt_test_topic", "123456", sizeof("123456"), MQTT_QOS2);
        LOG_I("publish %s", ret ? "succeed" : "failed");
        ret = ret;
        //com_sleep_ms(100);
    }
    com_sleep_s(2);
    ASSERT_INT_EQUAL(client.count, 10);
}

