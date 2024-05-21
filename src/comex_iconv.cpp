#include "comex_iconv.h"
#include "com_log.h"
#include "iconv.h"

static const char* ICONV_ENCODE_ARRAY[] =
{
    "UTF-32BE", "UTF-32LE", "UTF-32",
    "UTF-16BE", "UTF-16LE", "UTF-16",
    "UTF-8",
    "GB18030", "GBK", "CP939", "BIG5",
    "CP949", "CP932", "CP874", "CP1133", "CP1258", "CP864"
};

ComBytes comex_iconv_convert(const char* cs_to, const char* cs_from, const ComBytes& data)
{
    return comex_iconv_convert(cs_to, cs_from, data.getData(), data.getDataSize());
}

ComBytes comex_iconv_convert(const char* cs_to, const char* cs_from, const void* data, int data_size)
{
    if(cs_to == NULL || cs_from == NULL || data == NULL || data_size <= 0)
    {
        return ComBytes();
    }
    iconv_t handle = iconv_open(cs_to, cs_from);
    if(handle <= 0)
    {
        return ComBytes();
    }

    size_t size_src = (size_t)data_size;
    size_t size_src_remain = size_src;
    char* p_src = (char*)data;

    size_t size_dst = (size_src + 1) * 4;
    size_t size_dst_remian = size_dst;
    char* p_result = new char[size_dst_remian + 1];
    char* p_dst = p_result;

    int ret = iconv(handle, &p_src, &size_src_remain, &p_dst, &size_dst_remian);
    iconv_close(handle);
    if(ret == -1)
    {
        delete[] p_result;
        return ComBytes();
    }

    size_dst -= size_dst_remian;

    ComBytes result;
    result.append((uint8*)p_result, size_dst);

    delete[] p_result;
    return result;
}

std::string comex_iconv_dectect(const ComBytes& data)
{
    return comex_iconv_dectect(data.getData(), data.getDataSize());
}

std::string comex_iconv_dectect(const void* data, int data_size)
{
    if(data == NULL || data_size <= 0)
    {
        return std::string();
    }

    for(size_t i = 0; i < sizeof(ICONV_ENCODE_ARRAY) / sizeof(ICONV_ENCODE_ARRAY[0]); i++)
    {
        if(comex_iconv_convert("UTF-8", ICONV_ENCODE_ARRAY[i], data, data_size).empty() == false)
        {
            return ICONV_ENCODE_ARRAY[i];
        }
    }

    return std::string();
}

ComBytes comex_iconv_utf8_to_utf16(const ComBytes& utf8)
{
    return comex_iconv_convert(htons(0x1234) == 0x1234 ? "UTF-16BE" : "UTF-16LE",
                               "UTF-8",
                               utf8);
}

ComBytes comex_iconv_utf16_to_utf8(const ComBytes& utf16)
{
    return comex_iconv_convert("UTF-8",
                               htons(0x1234) == 0x1234 ? "UTF-16BE" : "UTF-16LE",
                               utf16);
}

ComBytes comex_iconv_utf8_to_utf32(const ComBytes& utf8)
{
    return comex_iconv_convert(htons(0x1234) == 0x1234 ? "UTF-32BE" : "UTF-32LE",
                               "UTF-8",
                               utf8);
}

ComBytes comex_iconv_utf32_to_utf8(const ComBytes& utf32)
{
    return comex_iconv_convert("UTF-8",
                               htons(0x1234) == 0x1234 ? "UTF-32BE" : "UTF-32LE",
                               utf32);
}

ComBytes comex_iconv_utf16_to_utf32(const ComBytes& utf16)
{
    return comex_iconv_convert(htons(0x1234) == 0x1234 ? "UTF-32BE" : "UTF-32LE",
                               htons(0x1234) == 0x1234 ? "UTF-16BE" : "UTF-16LE",
                               utf16);
}

ComBytes comex_iconv_utf32_to_utf16(const ComBytes& utf32)
{
    return comex_iconv_convert(htons(0x1234) == 0x1234 ? "UTF-16BE" : "UTF-16LE",
                               htons(0x1234) == 0x1234 ? "UTF-32BE" : "UTF-32LE",
                               utf32);
}

std::wstring comex_iconv_utf8_to_wstring(const ComBytes& utf8)
{
    std::wstring wstr;
    if(sizeof(wchar_t) == 2)
    {
        ComBytes utf16 = comex_iconv_utf8_to_utf16(utf8);
        wstr.append((wchar_t*)utf16.getData(), utf16.getDataSize() / 2);
    }
    else
    {
        ComBytes utf32 = comex_iconv_utf8_to_utf32(utf8);
        wstr.append((wchar_t*)utf32.getData(), utf32.getDataSize() / 4);
    }

    return wstr;
}

std::wstring comex_iconv_utf16_to_wstring(const ComBytes& utf16)
{
    std::wstring wstr;
    if(sizeof(wchar_t) == 2)
    {
        wstr.append((wchar_t*)utf16.getData(), utf16.getDataSize() / 2);
    }
    else
    {
        ComBytes utf32 = comex_iconv_utf16_to_utf32(utf16);
        wstr.append((wchar_t*)utf32.getData(), utf32.getDataSize() / 4);
    }

    return wstr;
}

std::wstring comex_iconv_utf32_to_wstring(const ComBytes& utf32)
{
    std::wstring wstr;
    if(sizeof(wchar_t) == 2)
    {
        ComBytes utf16 = comex_iconv_utf32_to_utf16(utf32);
        wstr.append((wchar_t*)utf16.getData(), utf16.getDataSize() / 2);
    }
    else
    {
        wstr.append((wchar_t*)utf32.getData(), utf32.getDataSize() / 4);
    }

    return wstr;
}

ComBytes comex_iconv_wstring_to_utf8(const std::wstring& wstr)
{
    ComBytes result;
    if(sizeof(wchar_t) == 2)
    {
        result.append((uint8*)wstr.data(), wstr.length() * 2);
        result = comex_iconv_utf16_to_utf8(result);
    }
    else
    {
        result.append((uint8*)wstr.data(), wstr.length() * 4);
        result = comex_iconv_utf32_to_utf8(result);
    }

    return result;
}

ComBytes comex_iconv_wstring_to_utf16(const std::wstring& wstr)
{
    ComBytes result;
    if(sizeof(wchar_t) == 2)
    {
        result.append((uint8*)wstr.data(), wstr.length() * 2);
    }
    else
    {
        result.append((uint8*)wstr.data(), wstr.length() * 4);
        result = comex_iconv_utf32_to_utf16(result);
    }

    return result;
}

ComBytes comex_iconv_wstring_to_utf32(const std::wstring& wstr)
{
    ComBytes result;
    if(sizeof(wchar_t) == 2)
    {
        result.append((uint8*)wstr.data(), wstr.length() * 2);
        result = comex_iconv_utf16_to_utf32(result);
    }
    else
    {
        result.append((uint8*)wstr.data(), wstr.length() * 4);
    }

    return result;
}

