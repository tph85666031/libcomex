#include "com_log.h"
#include "comex_poco.h"

void comex_poco_unit_test_suit(void** state)
{
    com_log_set_level(LOG_LEVEL_DEBUG);
    ComexPocoProxyServer server;
    server.setPort(8000, 8001, 8002, 8003);
    server.setCA("/data/libcomex/ca.crt", "/data/libcomex/ca.key");
    server.startServer();
    getchar();
}
