#include "comex_iconv.h"
#include "com_log.h"
#include "com_file.h"
#include "com_test.h"
#include <iostream>

void comex_iconv_unit_test_suit(void** state)
{
    com_log_init();
    ComBytes utf16 = comex_iconv_utf8_to_utf16(ComBytes("中文测试"));
    ComBytes utf32 = comex_iconv_utf16_to_utf32(utf16);

    std::wstring wstr = comex_iconv_utf32_to_wstring(utf32);
    ASSERT_TRUE(wstr == L"中文测试");

    wstr = comex_iconv_utf16_to_wstring(utf16);
    ASSERT_TRUE(wstr == L"中文测试");

    ComBytes utf8 = comex_iconv_wstring_to_utf8(L"中文测试");
    utf32 = comex_iconv_utf8_to_utf32(utf8);
    wstr = comex_iconv_utf32_to_wstring(utf32);
    ASSERT_TRUE(wstr == L"中文测试");

    LOG_I("encode=%s", comex_iconv_dectect("中文测试", sizeof("中文测试")).c_str());
    LOG_I("encode=%s", comex_iconv_dectect(comex_iconv_utf8_to_utf32(utf8)).c_str());
}

