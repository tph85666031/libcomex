#ifndef __COM_SESSION_H__
#define __COM_SESSION_H__

#ifdef ENABLE_DBUS

#include <dbus/dbus.h>
#include "com_base.h"

class ComexSession
{
public:
    std::string toJson();
public:
    std::string id;
    std::string seat;
    std::string seat_path;
    std::string user;
    std::string user_type;
    std::string user_path;
    std::string session_path;
    std::string tty;
    std::string display;
    std::string desktop;
    std::string remote_host;
    std::string remote_user;
    std::string state;
    std::string ui_type;
    bool acitve = false;
    int64 uid = -1;
};

#define QUERY_TYPE_PROPETY  1
#define QUERY_TYPE_SESSION  2

class QueryType
{
public:
    int type;
    uint32 serial;
};

class ComexSessionManager
{
public:
    ComexSessionManager();
    virtual ~ComexSessionManager();

    bool openDBus();
    void closeDBus();

    std::vector<ComexSession> getAllSessions();
    std::string getSessionIDByPID(int64 pid = -1);

    bool isSessionExist(const char* id);
    ComexSession getSession(const char* id);
    void showAllSesions();

    virtual void onSessionNew(ComexSession& session);
    virtual void onSessionRemoved(ComexSession& session);
private:
    static void ThreadListener(ComexSessionManager* ctx);
    static void ThreadEventDispatcher(ComexSessionManager* ctx);
    static DBusHandlerResult ListenerMessageCallback(DBusConnection* conn, DBusMessage* msg, void* user_data);
private:
    std::string getMessageValueAsString(DBusMessageIter& it);
    bool querySessions();
    bool queryProperties(const char* path);
    bool parseSessions(DBusMessage* msg);
    bool parseProperties(DBusMessage* msg);
private:
    DBusError err;
    DBusConnection* conn = NULL;
    std::atomic<bool> thread_listener_running = {false};
    std::atomic<bool> thread_event_dispatcher_running = {false};
    std::thread thread_listener;
    std::thread thread_dispatcher;

    std::mutex mutex_sessions;
    std::map<std::string, ComexSession> sessions;

    std::queue<std::string> event_new;
    std::queue<std::string> event_remove;
    ComSem sem_event;
    std::mutex mutex_event;

    std::mutex mutex_serial_id_propeties;
    int pos_serial_id = 0;
    uint32 serial_id_propeties[64];
    std::atomic<uint32> serial_id_sessions = {0};
};
#endif

#endif /* __COM_SESSION_H__ */

