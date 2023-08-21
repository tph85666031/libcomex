#ifdef ENABLE_DBUS
#include "comex_session.h"
#include "com_log.h"

void comex_session_unit_test_suit(void** state)
{
    CPPSessionManager mgr;
    LOG_I("my session=%s", mgr.getSessionIDByPID().c_str());
    mgr.showAllSesions();
}
#endif

