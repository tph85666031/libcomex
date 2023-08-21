#ifndef __COMEX_ICONV_H__
#define __COMEX_ICONV_H__

#include "com_base.h"

COM_EXPORT CPPBytes comex_iconv_convert(const char* cs_to, const char* cs_from, const void* data, int data_size);
COM_EXPORT CPPBytes comex_iconv_convert(const char* cs_to, const char* cs_from, const CPPBytes& data);

COM_EXPORT CPPBytes comex_iconv_utf8_to_utf16(const CPPBytes& utf8);
COM_EXPORT CPPBytes comex_iconv_utf16_to_utf8(const CPPBytes& utf16);
COM_EXPORT CPPBytes comex_iconv_utf8_to_utf32(const CPPBytes& utf8);
COM_EXPORT CPPBytes comex_iconv_utf32_to_utf8(const CPPBytes& utf32);
COM_EXPORT CPPBytes comex_iconv_utf16_to_utf32(const CPPBytes& utf16);
COM_EXPORT CPPBytes comex_iconv_utf32_to_utf16(const CPPBytes& utf32);

COM_EXPORT std::wstring comex_iconv_utf8_to_wstring(const CPPBytes& utf8);
COM_EXPORT std::wstring comex_iconv_utf16_to_wstring(const CPPBytes& utf16);
COM_EXPORT std::wstring comex_iconv_utf32_to_wstring(const CPPBytes& utf32);

COM_EXPORT CPPBytes comex_iconv_wstring_to_utf8(const std::wstring& wstr);
COM_EXPORT CPPBytes comex_iconv_wstring_to_utf16(const std::wstring& wstr);
COM_EXPORT CPPBytes comex_iconv_wstring_to_utf32(const std::wstring& wstr);

#endif /* __COMEX_ICONV_H__ */

