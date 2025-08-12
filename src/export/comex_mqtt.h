#ifndef __COMEX_MQTT_H__
#define __COMEX_MQTT_H__

#include "com_base.h"
#include "com_thread.h"
#include <queue>
#include <set>
#include <string>

#define MQTT_QOS0    0
#define MQTT_QOS1    1
#define MQTT_QOS2    2

class COM_EXPORT MqttProperty
{
public:
    MqttProperty();
    MqttProperty(const void* property);
    MqttProperty(const MqttProperty& property);
    MqttProperty& operator=(const MqttProperty& property);
    virtual ~MqttProperty();

    MqttProperty& setWillDelayInterval(uint32 time_s);
    MqttProperty& setSessionExpiryTime(uint32 time_s);
    MqttProperty& setMessageExpiryTime(uint32 time_s);
    MqttProperty& setResponseTopic(const char* topic);
    MqttProperty& setContentType(const char* type);
    MqttProperty& setAssignedClientID(const char* client_id);
    MqttProperty& setUserProp(const char* key, const char* value);
    uint32 getWillDelayInterval(uint32 default_val = 0);
    uint32 getSessionExpiryTime(uint32 default_val = 0);
    uint32 getMessageExpiryTime(uint32 default_val = 0);
    std::string getResponseTopic(std::string default_val = std::string());
    std::string getContentType(std::string default_val = std::string());
    std::string getAssignedClientID(std::string default_val = std::string());
    std::string getUserProp(const char* key, std::string default_val = std::string());

    void parse(const void* property);
    void* toProperty(bool recreate = false) const;
private:
    std::map<int, uint8> flags_uint8;
    std::map<int, uint16> flags_uint16;
    std::map<int, uint32> flags_uint32;
    std::map<int, uint32> flags_varint;
    std::map<int, std::string> flags_utf8;
    std::map<int, ComBytes> flags_array;
    std::map<std::string, std::string> flags_user;
    void* prop = NULL;
};

class COM_EXPORT MqttClient : public ComThreadPool
{
public:
    MqttClient();
    virtual ~MqttClient();
    MqttClient& setWillInfo(uint8* will_data, int will_data_size, const char* will_topic, int will_delay_s = 3, int will_qos = MQTT_QOS2, bool will_retain = true);
    MqttClient& setCleanSession(bool clean_session);
    MqttClient& setClientID(const char* client_id);
    MqttClient& setHost(const char* server_host);
    MqttClient& setPort(int server_port);
    MqttClient& setKeepalive(int keep_alive_ms);
    MqttClient& setUsername(const char* username);
    MqttClient& setPassword(const char* password);
    MqttClient& setCAInfo(const char* ca_file);
    MqttClient& setCertInfo(const char* cert_file, const char* key_file, const char* key_password);
    bool startClient(bool wait = false, int timeout_ms = 5000);
    void stopClient();
    bool subscribe(const char* topic, int qos = MQTT_QOS1);
    bool unsubscribe(const char* topic);
    bool publish(const char* topic, const void* data, int data_size, int qos = MQTT_QOS1, bool retain = false, const MqttProperty& property = MqttProperty());
    bool removeTopic(const char* topic);
    bool isConnected();

    virtual void onRecv(const std::string& topic, const uint8* data, int data_size, MqttProperty& prop);
    virtual void onConnectionChanged(bool connected, MqttProperty& prop);
private:
    bool openClient();
    void closeClient();
    void threadPoolRunner(Message& msg);
    static void ThreadLoop(MqttClient* ctx);
    static void MqttMessageCallback(void* mosq, void* userdata, const void* message, const void* props);
    static void MqttConnectCallback(void* mosq, void* userdata, int result, int flag, const void* props);
    static void MqttDisconnectCallback(void* mosq, void* userdata, int result, const void* props);
    static void MqttPublishCallback(void* mosq, void* userdata, int mid, int result, const void* props);
    static void MqttSubscribeCallback(void* mosq, void* userdata, int mid, int qos_count, const int* granted_qos, const void* props);
    static void MqttUnsubscribeCallback(void* mosq, void* userdata, int mid, const void* props);
    static int MqttPasswordCallback(char* buf, int size, int rwflag, void* userdata);
    static void MqttLogCallback(void* mosq, void* userdata, int level, const char* message);
private:
    std::mutex mutex_sub_topics;
    std::map<std::string, int> sub_topics;
    std::thread thread_loop;
    std::atomic<bool> connection_ready;
    ComSem sem_mqtt_conn;
    void* mosq = NULL;
    bool clean_session = true;
    std::string client_id;
    std::string server_host = "127.0.0.1";
    int server_port = 1883;
    int keepalive_ms = 1 * 1000;
    std::string username;
    std::string password;
    std::string ca_file;//PEM
    std::string cert_file;//PEM
    std::string key_file;//PEM
    std::string key_password;
    ComBytes will_data;
    std::string will_topic;
    int will_qos = MQTT_QOS1;
    int will_delay_s = 5;
    bool will_retain = false;
};

#endif /* __COMEX_MQTT_H__ */

