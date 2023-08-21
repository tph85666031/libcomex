#include "comex.h"
#include "com_log.h"
#include "com_test.h"

void comex_curl_unit_test_suit(void** state)
{
    CPPCurl curl;
    curl.enableDebug();
    curl.setVerifyCertDNS(true);
    HttpResponse response = curl.get("https://127.0.0.1:15362/eps/view/admin/stat");
    LOG_I("code=%ld,res=%s", response.code, response.msg.c_str());
}

