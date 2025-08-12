#include "com_base.h"
#include "com_log.h"
#include "com_test.h"

extern void comex_session_unit_test_suit(void** state);
extern void comex_mqtt_unit_test_suit(void** state);
extern void comex_liteipc_unit_test_suit(void** state);
extern void comex_openssl_unit_test_suit(void** state);
extern void comex_openssl_aes_unit_test_suit(void** state);
extern void comex_openssl_des_unit_test_suit(void** state);
extern void comex_openssl_des2_unit_test_suit(void** state);
extern void comex_openssl_des3_unit_test_suit(void** state);
extern void comex_smb_unit_test_suit(void** state);
extern void comex_nfs_unit_test_suit(void** state);
extern void comex_archive_unit_test_suit(void** state);
extern void comex_curl_unit_test_suit(void** state);
extern void comex_iconv_unit_test_suit(void** state);
extern void comex_podofo_unit_test_suit(void** state);
extern void comex_cairo_watermark_unit_test_suit(void** state);
extern void comex_magic_unit_test_suit(void** state);
extern void comex_socket_unit_test_suit(void** state);

CMUnitTest test_cases_comex_lib[] =
{
#if 0
    cmocka_unit_test(comex_openssl_unit_test_suit),
    cmocka_unit_test(comex_openssl_aes_unit_test_suit),
    cmocka_unit_test(comex_openssl_des2_unit_test_suit),
    cmocka_unit_test(comex_openssl_des3_unit_test_suit),
	cmocka_unit_test(comex_podofo_unit_test_suit),
	cmocka_unit_test(comex_archive_unit_test_suit),
	cmocka_unit_test(comex_iconv_unit_test_suit),
	cmocka_unit_test(comex_cairo_watermark_unit_test_suit)
#else
    cmocka_unit_test(comex_liteipc_unit_test_suit)
#endif
};

UNIT_TEST_LIB(test_cases_comex_lib);

