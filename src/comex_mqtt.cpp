#include "mosquitto.h"
#include "mqtt_protocol.h"
#include "com_thread.h"
#include "comex_mqtt.h"
#include "com_file.h"
#include "com_log.h"

typedef void (*fp_mqtt_v5_on_publish)(struct mosquitto*, void*, int, int, const mosquitto_property* props);
typedef void (*fp_mqtt_v5_on_connect)(struct mosquitto*, void*, int, int, const mosquitto_property* props);
typedef void (*fp_mqtt_v5_on_disconnect)(struct mosquitto*, void*, int, const mosquitto_property* props);
typedef void (*fp_mqtt_v5_on_message)(struct mosquitto*, void*, const struct mosquitto_message*, const mosquitto_property* props);
typedef void (*fp_mqtt_v5_on_subscribe)(struct mosquitto*, void*, int, int, const int*, const mosquitto_property* props);
typedef void (*fp_mqtt_v5_on_unsubscribe)(struct mosquitto*, void*, int, const mosquitto_property* props);
typedef void (*fp_mqtt_v5_on_log)(struct mosquitto*, void*, int, const char*);
static MqttInitializer initializer;

MqttInitializer::MqttInitializer()
{
    mosquitto_lib_init();
}

MqttInitializer::~MqttInitializer()
{
    mosquitto_lib_cleanup();
}

void comex_mqtt_global_init()
{
    mosquitto_lib_init();
}

void comex_mqtt_global_uninit()
{
    mosquitto_lib_cleanup();
}

MqttProperty::MqttProperty()
{
}

MqttProperty::MqttProperty(const void* property)
{
    parse(property);
}

MqttProperty::MqttProperty(const MqttProperty& property)
{
    if(this == &property)
    {
        return;
    }
    if(prop != NULL)
    {
        mosquitto_property_free_all((mosquitto_property**)&prop);
        prop = NULL;
    }
}

MqttProperty& MqttProperty::operator=(const MqttProperty& property)
{
    if(this == &property)
    {
        return *this;
    }
    if(prop != NULL)
    {
        mosquitto_property_free_all((mosquitto_property**)&prop);
        prop = NULL;
    }
    return *this;
}

MqttProperty::~MqttProperty()
{
    if(prop != NULL)
    {
        mosquitto_property_free_all((mosquitto_property**)&prop);
        prop = NULL;
    }
}

MqttProperty& MqttProperty::setSessionExpiryTime(uint32 time_s)
{
    session_expiry_time = time_s;
    flags.insert(MQTT_PROP_SESSION_EXPIRY_INTERVAL);
    return *this;
}

MqttProperty& MqttProperty::setMessageExpiryTime(uint32 time_s)
{
    message_expiry_time = time_s;
    flags.insert(MQTT_PROP_MESSAGE_EXPIRY_INTERVAL);
    return *this;
}

MqttProperty& MqttProperty::setResponseTopic(const char* topic)
{
    if(topic != NULL)
    {
        response_topic = topic;
        flags.insert(MQTT_PROP_RESPONSE_TOPIC);
    }
    return *this;
}

MqttProperty& MqttProperty::setContentType(const char* type)
{
    if(type != NULL)
    {
        content_type = type;
        flags.insert(MQTT_PROP_CONTENT_TYPE);
    }
    return *this;
}

MqttProperty& MqttProperty::setKV(const char* key, const char* value)
{
    if(key != NULL || value != NULL)
    {
        flags.insert(MQTT_PROP_USER_PROPERTY);
        kv[key] = value;
    }
    return  *this;
}

uint32 MqttProperty::getMessageExpiryTime()
{
    return message_expiry_time;
}

std::string MqttProperty::getAssignedClientID()
{
    return asigned_client_id;
}

std::string MqttProperty::getKV(const char* key, const char* default_val)
{
    if(key == NULL || kv.count(key) == 0)
    {
        return ((default_val == NULL) ? std::string() : default_val);
    }
    return kv[key];
}

void MqttProperty::parse(const void* property)
{
    if(property == NULL)
    {
        return;
    }
    mosquitto_property_read_int32((const mosquitto_property*)property, MQTT_PROP_MESSAGE_EXPIRY_INTERVAL, &message_expiry_time, false);
    mosquitto_property_read_int32((const mosquitto_property*)property, MQTT_PROP_SESSION_EXPIRY_INTERVAL, &session_expiry_time, false);

    char* value_str = NULL;
    if(mosquitto_property_read_string((const mosquitto_property*)property, MQTT_PROP_RESPONSE_TOPIC, &value_str, false) != NULL)
    {
        response_topic = value_str;
        free(value_str);
        value_str = NULL;
    }
    if(mosquitto_property_read_string((const mosquitto_property*)property, MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER, &value_str, false) != NULL)
    {
        asigned_client_id = value_str;
        free(value_str);
        value_str = NULL;
    }

    char* key_str = NULL;
    const mosquitto_property* p = mosquitto_property_read_string_pair((const mosquitto_property*)property,
                                  MQTT_PROP_USER_PROPERTY, &key_str,
                                  &value_str, false);
    while(p != NULL)
    {
        LOG_I("%s=%s", key_str, value_str);
        kv[key_str] = value_str;
        free(key_str);
        free(value_str);
        key_str = NULL;
        value_str = NULL;
        p = mosquitto_property_next(p);
        p = mosquitto_property_read_string_pair((const mosquitto_property*)p,
                                                MQTT_PROP_USER_PROPERTY, &key_str,
                                                &value_str, false);
    }
    return;
}

void* MqttProperty::toProperty(bool recreate) const
{
    if(recreate == true && prop != NULL)
    {
        mosquitto_property_free_all((mosquitto_property**)&prop);
    }

    if(prop != NULL)
    {
        return prop;
    }

    for(auto it = flags.begin(); it != flags.end(); it++)
    {
        switch(*it)
        {
            case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
                mosquitto_property_add_int32((mosquitto_property**)&prop, *it, message_expiry_time);
                break;
            case MQTT_PROP_SESSION_EXPIRY_INTERVAL:
                mosquitto_property_add_int32((mosquitto_property**)&prop, *it, session_expiry_time);
                break;
            case MQTT_PROP_RESPONSE_TOPIC:
                mosquitto_property_add_string((mosquitto_property**)&prop, *it, response_topic.c_str());
                break;
            case MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER:
                mosquitto_property_add_string((mosquitto_property**)&prop, *it, asigned_client_id.c_str());
                break;
            case MQTT_PROP_USER_PROPERTY:
                for(auto it2 = kv.begin(); it2 != kv.end(); it2++)
                {
                    mosquitto_property_add_string_pair((mosquitto_property**)&prop, *it, it2->first.c_str(), it2->second.c_str());
                }
                break;
        }
    }
    return prop;
}

MqttClient::MqttClient()
{
    clean_session = true;
    server_port = 0;
    will_qos = MQTT_QOS0;
    will_retain = false;
	connection_ready = false;
    mosq = NULL;
}

MqttClient::~MqttClient()
{
    stopClient();
}

void MqttClient::MqttMessageCallback(void* mosq, void* userdata,
                                     const void* message,
                                     const void* props)
{
    const struct mosquitto_message* p_message = (const struct mosquitto_message*)message;
    if(userdata == NULL || p_message == NULL || p_message->topic == NULL)
    {
        LOG_E("arg incorrect");
        return;
    }

    MqttClient* ctx = (MqttClient*)userdata;
    LOG_D("[%s]mqtt got message,topic=%s,prop=%p", ctx->client_id.c_str(), p_message->topic, props);
    if(p_message->payload == NULL ||  p_message->payloadlen <= 0 || p_message->topic == NULL)
    {
        LOG_I("[%s]topic removed:%s", ctx->client_id.c_str(), p_message->topic);
        return;
    }

    Message msg;
    msg.set("topic", (const char*)p_message->topic);
    msg.set("data", (uint8*)p_message->payload, p_message->payloadlen);

    mosquitto_property* p_prop_copy = NULL;
    if(props != NULL)
    {
        mosquitto_property_copy_all(&p_prop_copy, (mosquitto_property*)props);
        msg.setPtr("prop", p_prop_copy);
    }

    if(ctx->pushPoolMessage(std::move(msg)) == false && p_prop_copy != NULL)
    {
        mosquitto_property_free_all(&p_prop_copy);
    }
}

void MqttClient::MqttConnectCallback(void* mosq, void* userdata, int result, const void* props)
{
    if(userdata == NULL)
    {
        return;
    }
    MqttClient* ctx = (MqttClient*)userdata;
    ctx->connection_ready = (result == 0);
    ctx->sem_mqtt_conn.post();
    LOG_D("[%s]connect error_str=%s,result=%d,prop=%p", ctx->client_id.c_str(), mosquitto_strerror(result), result, props);
    if(ctx->connection_ready)
    {
        ctx->mutex_sub_topics.lock();
        for(auto it = ctx->sub_topics.begin(); it != ctx->sub_topics.end(); it++)
        {
            mosquitto_subscribe((struct mosquitto*)mosq, NULL, it->first.c_str(), it->second);
        }
        ctx->mutex_sub_topics.unlock();
    }
    MqttProperty p(props);
    ctx->onConnectionChanged(ctx->connection_ready, p);
}

void MqttClient::MqttDisconnectCallback(void* mosq, void* userdata, int result, const void* props)
{
    if(userdata == NULL)
    {
        return;
    }
    MqttClient* ctx = (MqttClient*)userdata;
    if(result)
    {
        LOG_D("[%s]connect error_str=%s,result=%d", ctx->client_id.c_str(), mosquitto_strerror(result), result);
    }
    else
    {
        LOG_D("[%s]connect error_str=%s", ctx->client_id.c_str(), mosquitto_strerror(result));
    }
    ctx->connection_ready = false;
    MqttProperty p(props);
    ctx->onConnectionChanged(ctx->connection_ready, p);
}

void MqttClient::MqttPublishCallback(void* mosq, void* userdata, int mid, const void* props)
{
    LOG_D("publish succeed,mosq=%p,mid=%d", mosq, mid);
}

void MqttClient::MqttSubscribeCallback(void* mosq, void* userdata, int mid, int qos_count, const int* granted_qos, const void* props)
{
    LOG_D("subsribe succeed,mosq=%p,mid=%d", mosq, mid);
}

void MqttClient::MqttUnsubscribeCallback(void* mosq, void* userdata, int mid, const void* props)
{
    LOG_D("unsubsribe succeed,mosq=%p,mid=%d", mosq, mid);
}

int MqttClient::MqttPasswordCallback(char* buf, int size, int rwflag, void* userdata)
{
    if(userdata == NULL || buf == NULL || size <= 0)
    {
        return -1;
    }
    MqttClient* ctx = (MqttClient*)userdata;
    return snprintf(buf, size, "%s", ctx->key_password.c_str());
}

void MqttClient::MqttLogCallback(void* mosq, void* userdata, int level, const char* message)
{
    if(message == NULL)
    {
        return;
    }
    if(level == MOSQ_LOG_ERR)
    {
        LOG_E("%s", message);
    }
    else if(level == MOSQ_LOG_WARNING)
    {
        LOG_W("%s", message);
    }
    else
    {
        LOG_D("%s", message);
    }
}

void MqttClient::ThreadLoop(MqttClient* ctx)
{
    if(ctx == NULL)
    {
        return;
    }
    LOG_I("called");
    mosquitto_connect_async((struct mosquitto*)ctx->mosq, ctx->server_host.c_str(), ctx->server_port, ctx->keepalive_ms);
    int ret = mosquitto_loop_forever((struct mosquitto*)ctx->mosq, -1, 1);
    LOG_I("quit,ret=%d:%s", ret, mosquitto_strerror(ret));
}

void MqttClient::threadPoolRunner(Message& msg)
{
    std::string topic = msg.getString("topic");
    int size = 0;
    uint8* data = msg.getBytes("data", size);
    void* prop = msg.getPtr("prop");
    MqttProperty p(prop);
    onRecv(topic, data, size, p);
    mosquitto_property_free_all((mosquitto_property**)&prop);
}

MqttClient& MqttClient::setWillInfo(uint8* will_data, int will_data_size,
                                    const char* will_topic, int will_qos, bool will_retain)
{
    this->will_data = CPPBytes(will_data, will_data_size);
    if(will_topic != NULL)
    {
        this->will_topic = will_topic;
    }
    this->will_qos = will_qos;
    this->will_retain = will_retain;
    return *this;
}

MqttClient& MqttClient::setCleanSession(bool clean_session)
{
    this->clean_session = clean_session;
    return *this;
}

MqttClient& MqttClient::setClientID(const char* client_id)
{
    if(client_id != NULL)
    {
        this->client_id = client_id;
    }
    return *this;
}

MqttClient& MqttClient::setHost(const char* server_host)
{
    if(server_host != NULL)
    {
        this->server_host = server_host;
    }
    return *this;
}

MqttClient& MqttClient::setPort(int server_port)
{
    this->server_port = server_port;
    return *this;
}

MqttClient& MqttClient::setKeepalive(int keepalive_ms)
{
    this->keepalive_ms = keepalive_ms;
    return *this;
}

MqttClient& MqttClient::setUsername(const char* username)
{
    if(username != NULL)
    {
        this->username = username;
    }
    return *this;
}

MqttClient& MqttClient::setPassword(const char* password)
{
    if(password != NULL)
    {
        this->password = password;
    }
    return *this;
}

MqttClient& MqttClient::setCAInfo(const char* ca_file)
{
    if(ca_file != NULL)
    {
        this->ca_file = ca_file;
    }
    return *this;
}

MqttClient& MqttClient::setCertInfo(const char* ca_file, const char* key_file, const char* key_password)
{
    if(ca_file != NULL)
    {
        this->ca_file = ca_file;
    }
    if(key_file != NULL)
    {
        this->key_file = key_file;
    }
    if(key_password != NULL)
    {
        this->key_password = key_password;
    }
    return *this;
}

bool MqttClient::openClient()
{
    int ret;
    LOG_D("username=%s,client_id=%s,host=%s,port=%d",
          username.c_str(), client_id.c_str(),
          server_host.c_str(), server_port);
    mosq = mosquitto_new(client_id.empty() ? NULL : client_id.c_str(), clean_session, this);
    if(mosq == NULL)
    {
        LOG_E("mqtt init error");
        return false;
    }

    if(username.empty() == false && password.empty() == false)
    {
        ret = mosquitto_username_pw_set((struct mosquitto*)mosq, username.c_str(), password.c_str());
        if(ret != MOSQ_ERR_SUCCESS)
        {
            LOG_E("ret=%s", mosquitto_strerror(ret));
            closeClient();
            return false;
        }
    }

    if(ca_file.empty() == false || (cert_file.empty() == false && key_file.empty() == false))
    {
        std::string caPath;
        if(ca_file.empty() == false)
        {
            caPath = com_path_dir(ca_file.c_str());
        }
        LOG_D("ca_file=%s,caPath=%s,cert=%s,key=%s", ca_file.c_str(), caPath.c_str(), cert_file.c_str(), key_file.c_str());
        mosquitto_tls_insecure_set((struct mosquitto*)mosq, true);
        //mosquitto_tls_opts_set((struct mosquitto*)mosq, 1, "tlsv1", NULL);
        ret = mosquitto_tls_set((struct mosquitto*)mosq,
                                ca_file.empty() ? NULL : ca_file.c_str(),
                                caPath.empty() ? NULL : caPath.c_str(),
                                cert_file.empty() ? NULL : cert_file.c_str(),
                                key_file.empty() ? NULL : key_file.c_str(),
                                MqttPasswordCallback);

        if(ret != MOSQ_ERR_SUCCESS)
        {
            LOG_E("tls set failed, ret=%s", mosquitto_strerror(ret));
            closeClient();
            return false;
        }
    }

    mosquitto_user_data_set((struct mosquitto*)mosq, this);
    mosquitto_message_retry_set((struct mosquitto*)mosq, 10);

    mosquitto_int_option((struct mosquitto*)mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
    mosquitto_publish_v5_callback_set((struct mosquitto*)mosq, (fp_mqtt_v5_on_publish)MqttPublishCallback);
    mosquitto_connect_v5_callback_set((struct mosquitto*)mosq, (fp_mqtt_v5_on_connect)MqttConnectCallback);
    mosquitto_disconnect_v5_callback_set((struct mosquitto*)mosq, (fp_mqtt_v5_on_disconnect)MqttDisconnectCallback);
    mosquitto_message_v5_callback_set((struct mosquitto*)mosq, (fp_mqtt_v5_on_message)MqttMessageCallback);
    mosquitto_subscribe_v5_callback_set((struct mosquitto*)mosq, (fp_mqtt_v5_on_subscribe)MqttSubscribeCallback);
    mosquitto_unsubscribe_v5_callback_set((struct mosquitto*)mosq, (fp_mqtt_v5_on_unsubscribe)MqttUnsubscribeCallback);
    mosquitto_log_callback_set((struct mosquitto*)mosq, (fp_mqtt_v5_on_log)MqttLogCallback);

    if(will_data.empty() || will_topic.empty())
    {
        mosquitto_will_clear((struct mosquitto*)mosq);
    }
    else
    {
        mosquitto_will_set((struct mosquitto*)mosq, will_topic.c_str(),
                           will_data.getDataSize(),
                           will_data.getData(),
                           will_qos,
                           will_retain);
    }
    mosquitto_reconnect_delay_set((struct mosquitto*)mosq, 1, 1, false);
#if defined(_WIN32) || defined(_WIN64)
    thread_loop = std::thread(ThreadLoop, this);
#else
    //先mosquitto_loop_start后mosquitto_connect_async,否则broker上线后基于socket的client无法连接
    mosquitto_loop_start((struct mosquitto*)mosq);
    mosquitto_connect_async((struct mosquitto*)mosq, server_host.c_str(), server_port, keepalive_ms);
#endif
    LOG_D("init done");
    return true;
}

void MqttClient::closeClient()
{
    if(mosq != NULL)
    {
        mosquitto_disconnect((struct mosquitto*)mosq);
        mosquitto_loop_stop((struct mosquitto*)mosq, false);
        mosquitto_destroy((struct mosquitto*)mosq);
        mosq = NULL;
    }
    if(thread_loop.joinable())
    {
        thread_loop.join();
    }
}

bool MqttClient::startClient(bool wait, int timeout_ms)
{
    LOG_D("called");
    if(client_id.empty())
    {
        LOG_W("mqtt client_id not specified");
        //return false;
    }

    if(server_host.empty())
    {
        LOG_E("mqtt host not specified");
        return false;
    }

    startThreadPool();
    if(openClient() == false)
    {
        return false;
    }

    if(wait && timeout_ms >= 0)
    {
        sem_mqtt_conn.wait(timeout_ms);

        if(connection_ready == false)
        {
            LOG_W("connection timeout");
            return false;
        }
    }

    return true;
}

void MqttClient::stopClient()
{
    LOG_D("called");
    closeClient();
    stopThreadPool();
}

bool MqttClient::subscribe(const char* topic, int qos)
{
    if(qos > MQTT_QOS2 || qos < MQTT_QOS0 || topic == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    int ret = mosquitto_subscribe((struct mosquitto*)mosq, NULL, topic, qos);
    LOG_D("[%s]topic subscribed=%s,ret=%d", client_id.c_str(), topic, ret);
    mutex_sub_topics.lock();
    sub_topics[topic] = qos;
    mutex_sub_topics.unlock();
    return ret == MOSQ_ERR_SUCCESS ? true : false;
}

bool MqttClient::unsubscribe(const char* topic)
{
    if(topic == NULL)
    {
        LOG_E("topic incorrect");
        return false;
    }

    int ret = mosquitto_unsubscribe((struct mosquitto*)mosq, NULL, topic);
    LOG_D("[%s]topic unsubscribed=%s,ret=%d", client_id.c_str(), topic, ret);
    mutex_sub_topics.lock();
    auto it = sub_topics.find(topic);
    if(it != sub_topics.end())
    {
        sub_topics.erase(it);

    }
    mutex_sub_topics.unlock();
    return ret == MOSQ_ERR_SUCCESS ? true : false;
}

bool MqttClient::publish(const char* topic, const void* data, int data_size,
                         int qos, bool retain, const MqttProperty& property)
{
    if(topic == NULL)
    {
        LOG_E("topic incorrect");
        return false;
    }
    if(data_size > 0 && data == NULL)
    {
        LOG_E("data incorrect");
        return false;
    }
    LOG_D("[%s]publish topic=%s", client_id.c_str(), topic);
    int ret = mosquitto_publish_v5((struct mosquitto*)mosq, NULL, topic, data_size, data,
                                   qos, retain, (const mosquitto_property*)property.toProperty());
    if(ret != MOSQ_ERR_SUCCESS)
    {
        LOG_W("[%s]failed, need retry, ret=%d, errno=%d:%s", client_id.c_str(), ret, errno, strerror(errno));
        return false;
    }
    return true;
}

bool MqttClient::removeTopic(const char* topic)
{
    return publish(topic, NULL, 0, 0, true);
}

bool MqttClient::isConnected()
{
    return connection_ready;
}

void MqttClient::onRecv(const std::string& topic, const uint8* data, int data_size, MqttProperty& prop)
{
}

void MqttClient::onConnectionChanged(bool connected, MqttProperty& prop)
{
}

