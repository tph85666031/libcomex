#ifdef ENABLE_DBUS
#include <iostream>
#include "com_base.h"
#include "com_log.h"
#include "com_file.h"
#include "CJsonObject.h"
#include "comex_session.h"

std::string CPPSession::toJson()
{
    CJsonObject json;
    json.Add("ID", id);
    json.Add("Acitve", acitve);
    json.Add("UID", uid);
    json.Add("User", user);
    json.Add("UserPath", user_path);
    json.Add("UserType", user_type);
    json.Add("Seat", seat);
    json.Add("SeatPath", seat_path);
    json.Add("SessionPath", session_path);
    json.Add("TTY", tty);
    json.Add("Display", display);
    json.Add("RemoteHost", remote_host);
    json.Add("RemoteUser", remote_user);
    json.Add("State", state);
    json.Add("UIType", ui_type);

    return json.ToString();
}

CPPSessionManager::CPPSessionManager()
{
    memset(serial_id_propeties, 0, sizeof(serial_id_propeties));
    openDBus();
    querySessions();
}

CPPSessionManager::~CPPSessionManager()
{
    closeDBus();
}

bool CPPSessionManager::openDBus()
{
    dbus_error_init(&err);
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if(dbus_error_is_set(&err))
    {
        LOG_E("dbus err, error=%s:%s", err.name, err.message);
        dbus_error_free(&err);
    }
    if(conn == NULL)
    {
        LOG_E("dbus connection failed");
        return false;
    }

    dbus_threads_init_default();
    dbus_bus_add_match(conn, "type='signal',interface='org.freedesktop.login1.Manager',member='SessionNew'", &err);
    if(dbus_error_is_set(&err))
    {
        LOG_E("dbus err, error=%s:%s", err.name, err.message);
        dbus_error_free(&err);
    }
    dbus_bus_add_match(conn, "type='signal',interface='org.freedesktop.login1.Manager',member='SessionRemoved'", &err);
    if(dbus_error_is_set(&err))
    {
        LOG_E("dbus err, error=%s:%s", err.name, err.message);
        dbus_error_free(&err);
    }
    dbus_connection_add_filter(conn, ListenerMessageCallback, this, NULL);
    dbus_connection_flush(conn);

    thread_listener_running = true;
    thread_listener = std::thread(ThreadListener, this);

    thread_event_dispatcher_running = true;
    thread_dispatcher = std::thread(ThreadEventDispatcher, this);
    return true;
}

void CPPSessionManager::closeDBus()
{
    thread_event_dispatcher_running = false;
    if(thread_dispatcher.joinable())
    {
        thread_dispatcher.join();
    }

    //dbus_connection_close(conn);
    thread_listener_running = false;
    if(thread_listener.joinable())
    {
        thread_listener.join();
    }
}

std::string CPPSessionManager::getMessageValueAsString(DBusMessageIter& it)
{
    int type = dbus_message_iter_get_arg_type(&it);
    if(type == DBUS_TYPE_BYTE)
    {
        uint8 val = 0;
        dbus_message_iter_get_basic(&it, &val);
        return std::to_string(val);
    }
    else if(type == DBUS_TYPE_BOOLEAN)
    {
        bool val = false;
        dbus_message_iter_get_basic(&it, &val);
        return val ? "true" : "false";
    }
    else if(type == DBUS_TYPE_INT16)
    {
        int16 val = 0;
        dbus_message_iter_get_basic(&it, &val);
        return std::to_string(val);
    }
    else if(type == DBUS_TYPE_UINT16)
    {
        uint16 val = 0;
        dbus_message_iter_get_basic(&it, &val);
        return std::to_string(val);
    }
    else if(type == DBUS_TYPE_INT32)
    {
        int32 val = 0;
        dbus_message_iter_get_basic(&it, &val);
        return std::to_string(val);
    }
    else if(type == DBUS_TYPE_UINT32)
    {
        uint32 val = 0;
        dbus_message_iter_get_basic(&it, &val);
        return std::to_string(val);
    }
    else if(type == DBUS_TYPE_INT64)
    {
        int64 val = 0;
        dbus_message_iter_get_basic(&it, &val);
        return std::to_string(val);
    }
    else if(type == DBUS_TYPE_UINT64)
    {
        uint64 val = 0;
        dbus_message_iter_get_basic(&it, &val);
        return std::to_string(val);
    }
    else if(type == DBUS_TYPE_DOUBLE)
    {
        double val = 0;
        dbus_message_iter_get_basic(&it, &val);
        return std::to_string(val);
    }
    else if(type == DBUS_TYPE_STRING || type == DBUS_TYPE_OBJECT_PATH)
    {
        char* val = NULL;
        dbus_message_iter_get_basic(&it, &val);
        if(val == NULL)
        {
            return std::string();
        }
        return val;
    }
    else
    {
        //LOG_E("unknown type:%c", (char)type);
    }
    return std::string();
}

bool CPPSessionManager::querySessions()
{
    DBusMessage* msg = dbus_message_new_method_call("org.freedesktop.login1",
                       "/org/freedesktop/login1",
                       "org.freedesktop.login1.Manager",
                       "ListSessions");
    if(msg == NULL)
    {
        LOG_E("failed");
        return false;
    }
    uint32 serial = 0;
    if(dbus_connection_send(conn, msg, &serial) == false)
    {
        return false;
    }
    serial_id_sessions = serial;
    dbus_connection_flush(conn);
    return true;
}

bool CPPSessionManager::queryProperties(const char* path)
{
    if(path == NULL)
    {
        return false;
    }
    DBusMessage* msg = dbus_message_new_method_call("org.freedesktop.login1",
                       path,
                       "org.freedesktop.DBus.Properties",
                       "GetAll");
    if(msg == NULL)
    {
        return false;
    }

    DBusMessageIter it;
    dbus_message_iter_init_append(msg, &it);
    const char* interface = "";
    dbus_message_iter_append_basic(&it, DBUS_TYPE_STRING, &interface);
    uint32 serial = 0;
    if(dbus_connection_send(conn, msg, &serial) == false)
    {
        return false;
    }
    mutex_serial_id_propeties.lock();
    serial_id_propeties[pos_serial_id] = serial;
    pos_serial_id = (pos_serial_id + 1) % (sizeof(serial_id_propeties) / sizeof(serial_id_propeties[0]));
    mutex_serial_id_propeties.unlock();

    dbus_connection_flush(conn);
    return true;
}

bool CPPSessionManager::parseSessions(DBusMessage* msg)
{
    LOG_I("called");
    if(msg == NULL)
    {
        LOG_E("failed");
        return false;
    }

    DBusMessageIter it0;
    dbus_message_iter_init(msg, &it0);
    int type = dbus_message_iter_get_arg_type(&it0);
    if(type != DBUS_TYPE_ARRAY)
    {
        LOG_E("type incorrect:%d", type);
        return false;
    }

    DBusMessageIter it1;
    dbus_message_iter_recurse(&it0, &it1);
    std::lock_guard<std::mutex> lck(mutex_sessions);
    do
    {
        type = dbus_message_iter_get_arg_type(&it1);
        if(type != DBUS_TYPE_STRUCT)
        {
            LOG_E("type incorrect:%d", type);
            return false;
        }

        DBusMessageIter it2;
        dbus_message_iter_recurse(&it1, &it2);

        //session id
        std::string id = getMessageValueAsString(it2);

        if(sessions.count(id) == 0)
        {
            sessions[id] = CPPSession();
        }

        CPPSession& session = sessions[id];
        session.id = id;

        //uid
        if(dbus_message_iter_has_next(&it2) == false)
        {
            continue;
        }
        dbus_message_iter_next(&it2);
        session.uid = strtol(getMessageValueAsString(it2).c_str(), NULL, 10);

        //user
        if(dbus_message_iter_has_next(&it2) == false)
        {
            continue;
        }
        dbus_message_iter_next(&it2);
        session.user = getMessageValueAsString(it2);

        //seat
        if(dbus_message_iter_has_next(&it2) == false)
        {
            continue;
        }
        dbus_message_iter_next(&it2);
        session.seat = getMessageValueAsString(it2);

        //path
        if(dbus_message_iter_has_next(&it2) == false)
        {
            continue;
        }
        dbus_message_iter_next(&it2);
        session.session_path = getMessageValueAsString(it2);

        queryProperties(session.session_path.c_str());

        if(dbus_message_iter_has_next(&it1) == false)
        {
            break;
        }
        dbus_message_iter_next(&it1);
    }
    while(true);

    return true;
}

bool CPPSessionManager::parseProperties(DBusMessage* msg)
{
    if(msg == NULL)
    {
        LOG_E("failed");
        return false;
    }

    CPPSession session;
    DBusMessageIter it0;
    dbus_message_iter_init(msg, &it0);

    int type = dbus_message_iter_get_arg_type(&it0);
    if(type != DBUS_TYPE_ARRAY)
    {
        LOG_E("type incorrect:%d", type);
        return false;
    }

    DBusMessageIter it1;
    dbus_message_iter_recurse(&it0, &it1);

    do
    {
        type = dbus_message_iter_get_arg_type(&it1);
        DBusMessageIter it2;
        dbus_message_iter_recurse(&it1, &it2);
        std::string key = getMessageValueAsString(it2);

        dbus_message_iter_next(&it2);

        DBusMessageIter it3;
        dbus_message_iter_recurse(&it2, &it3);

        int type = dbus_message_iter_get_arg_type(&it3);
        if(type == DBUS_TYPE_STRUCT)
        {
            if(key == "User")
            {
                DBusMessageIter it4;
                dbus_message_iter_recurse(&it3, &it4);
                std::string value = getMessageValueAsString(it4);
                session.uid = strtoll(value.c_str(), NULL, 10);

                dbus_message_iter_next(&it4);
                value = getMessageValueAsString(it4);
                session.user_path = value;
            }
            else if(key == "Seat")
            {
                DBusMessageIter it4;
                dbus_message_iter_recurse(&it3, &it4);
                type = dbus_message_iter_get_arg_type(&it4);
                std::string value = getMessageValueAsString(it4);
                session.seat = value;

                dbus_message_iter_next(&it4);
                value = getMessageValueAsString(it4);
                session.seat_path = value;
            }
        }
        else
        {
            std::string value = getMessageValueAsString(it3);
            if(key == "Id")
            {
                session.id = value;
            }
            else if(key == "Name")
            {
                session.user = value;
            }
            else if(key == "Display")
            {
                session.display = value;
            }
            else if(key == "TTY")
            {
                session.tty = value;
            }
            else if(key == "State")
            {
                session.state = value;
            }
            else if(key == "Type")
            {
                session.ui_type = value;
            }
            else if(key == "Desktop")
            {
                session.desktop = value;
            }
            else if(key == "RemoteHost")
            {
                session.remote_host = value;
            }
            else if(key == "RemoteUser")
            {
                session.remote_user = value;
            }
            else if(key == "Class")
            {
                session.user_type = value;
            }
            else if(key == "Active")
            {
                session.acitve = (value == "true");
            }
        }

        if(dbus_message_iter_has_next(&it1) == false)
        {
            break;
        }
        dbus_message_iter_next(&it1);
    }
    while(true);

    bool is_session_new = false;
    mutex_sessions.lock();
    if(sessions.count(session.id) > 0)
    {
        CPPSession& session_old = sessions[session.id];
        session.user = session_old.user;
        session.session_path = session_old.session_path;
        session.seat = session_old.seat;
        session.uid = session_old.uid;
        sessions[session.id] = session;
    }
    else
    {
        sessions[session.id] = session;
        is_session_new = true;
    }
    mutex_sessions.unlock();

    if(is_session_new)
    {
        mutex_event.lock();
        event_new.push(session.id);
        mutex_event.unlock();
        sem_event.post();
    }
    return true;
}

void CPPSessionManager::onSessionNew(CPPSession& session)
{
    LOG_I("%s", session.toJson().c_str());
}

void CPPSessionManager::onSessionRemoved(CPPSession& session)
{
    LOG_I("%s", session.toJson().c_str());
}

bool CPPSessionManager::isSessionExist(const char* id)
{
    if(id == NULL)
    {
        return false;
    }
    std::lock_guard<std::mutex> lck(mutex_sessions);
    return sessions.count(id) > 0;
}

CPPSession CPPSessionManager::getSession(const char* id)
{
    if(id == NULL)
    {
        return CPPSession();
    }
    std::lock_guard<std::mutex> lck(mutex_sessions);
    if(sessions.count(id) == 0)
    {
        return CPPSession();
    }
    return sessions[id];
}

std::vector<CPPSession> CPPSessionManager::getAllSessions()
{
    std::vector<CPPSession> values;

    std::lock_guard<std::mutex> lck(mutex_sessions);
    for(auto it = sessions.begin(); it != sessions.end(); it++)
    {
        values.push_back(it->second);
    }
    return values;
}

std::string CPPSessionManager::getSessionIDByPID(int64 pid)
{
    if(pid < 0)
    {
        static std::string my_session_id;
        if(my_session_id.empty())
        {
            my_session_id = com_file_readall("/proc/self/sessionid").toString();
        }
        return my_session_id;
    }

    return com_file_readall(com_string_format("/proc/%lld/sessionid", pid).c_str()).toString();
}

void CPPSessionManager::ThreadListener(CPPSessionManager* ctx)
{
    if(ctx == NULL)
    {
        return;
    }
    while(ctx->thread_listener_running)
    {
        dbus_connection_read_write_dispatch(ctx->conn, 1000);
    }
    LOG_I("quit");
    return;
}

void CPPSessionManager::showAllSesions()
{
    std::lock_guard<std::mutex> lck(mutex_sessions);
    for(auto it = sessions.begin(); it != sessions.end(); it++)
    {
        CPPSession& session = it->second;
        LOG_I("%s", session.toJson().c_str());
    }
}

void CPPSessionManager::ThreadEventDispatcher(CPPSessionManager* ctx)
{
    if(ctx == NULL)
    {
        return;
    }
    while(ctx->thread_event_dispatcher_running)
    {
        if(ctx->sem_event.wait(1000) == false)
        {
            continue;
        }
        do
        {
            ctx->mutex_event.lock();
            if(ctx->event_new.empty())
            {
                ctx->mutex_event.unlock();
                break;
            }
            std::string session_id = ctx->event_new.front();
            ctx->event_new.pop();
            ctx->mutex_event.unlock();

            ctx->mutex_sessions.lock();
            if(ctx->sessions.count(session_id) == 0)
            {
                ctx->mutex_sessions.unlock();
                continue;
            }
            CPPSession session = ctx->sessions[session_id];
            ctx->mutex_sessions.unlock();
            ctx->onSessionNew(session);

        }
        while(ctx->thread_event_dispatcher_running);

        do
        {
            ctx->mutex_event.lock();
            if(ctx->event_remove.empty())
            {
                ctx->mutex_event.unlock();
                break;
            }
            std::string session_id = ctx->event_remove.front();
            ctx->event_remove.pop();
            ctx->mutex_event.unlock();

            ctx->mutex_sessions.lock();
            CPPSession session;
            session.id = session_id;
            auto it = ctx->sessions.find(session_id);
            if(it != ctx->sessions.end())
            {
                session = it->second;
                ctx->sessions.erase(it);
            }
            ctx->mutex_sessions.unlock();
            session.state = "closing";
            session.acitve = false;
            ctx->onSessionRemoved(session);
        }
        while(ctx->thread_event_dispatcher_running);
    }
    LOG_I("quit");
    return;
}

DBusHandlerResult CPPSessionManager::ListenerMessageCallback(DBusConnection* conn, DBusMessage* msg, void* user_data)
{
    if(conn == NULL || msg == NULL || user_data == NULL)
    {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    CPPSessionManager* ctx = (CPPSessionManager*)user_data;
#if 0
    LOG_I("called,type=%d,member=%s,interface=%s,sender=%s,signature=%s,serial=%u,destination=%s,path=%s",
          dbus_message_get_type(msg),
          dbus_message_get_member(msg),
          dbus_message_get_interface(msg),
          dbus_message_get_sender(msg),
          dbus_message_get_signature(msg),
          dbus_message_get_reply_serial(msg),
          dbus_message_get_destination(msg),
          dbus_message_get_path(msg));
#endif
    int type = dbus_message_get_type(msg);
    if(type == DBUS_MESSAGE_TYPE_SIGNAL)
    {
        const char* method = dbus_message_get_member(msg);
        const char* interafce = dbus_message_get_interface(msg);
        if(method == NULL)
        {
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
        if(com_string_equal(interafce, "org.freedesktop.login1.Manager") == false)
        {
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
        DBusMessageIter it;
        dbus_message_iter_init(msg, &it);
        std::string id = ctx->getMessageValueAsString(it);
        dbus_message_iter_next(&it);
        std::string path = ctx->getMessageValueAsString(it);
        if(id.empty() && path.empty())
        {
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        if(com_string_equal(method, "SessionNew"))
        {
            ctx->queryProperties(path.c_str());
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        else if(com_string_equal(method, "SessionRemoved"))
        {
            ctx->mutex_event.lock();
            ctx->event_remove.push(id);
            ctx->mutex_event.unlock();
            ctx->sem_event.post();
            return DBUS_HANDLER_RESULT_HANDLED;
        }
    }
    else if(type == DBUS_MESSAGE_TYPE_METHOD_RETURN)
    {
        uint32 serial = dbus_message_get_reply_serial(msg);
        if(serial == ctx->serial_id_sessions)
        {
            ctx->parseSessions(msg);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        else
        {
            std::lock_guard<std::mutex> lck(ctx->mutex_serial_id_propeties);
            for(size_t i = 0; i < sizeof(ctx->serial_id_propeties) / sizeof(ctx->serial_id_propeties[0]); i++)
            {
                if(ctx->serial_id_propeties[i] == serial)
                {
                    ctx->parseProperties(msg);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
            }
        }
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
#endif
