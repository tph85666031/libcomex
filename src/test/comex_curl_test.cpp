#include "comex_curl.h"
#include "com_log.h"
#include "com_test.h"

void comex_curl_unit_test_suit(void** state)
{
    ComexCurl curl;
    curl.enableDebug();
    curl.setVerifyCertDNS(true);
    HttpResponse response = curl.get("https://www.baidu.com");
    LOG_I("code=%ld,res=%s", response.code, response.msg.c_str());
}

