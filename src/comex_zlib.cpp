#include "zlib.h"
#include "comex_zlib.h"

CPPBytes comex_zlib_compress(const uint8* data, int data_size)
{
    if(data == NULL || data_size <= 0)
    {
        return CPPBytes();
    }
    uLong size = compressBound(data_size);
    if(size <= 0)
    {
        return CPPBytes();
    }

    uint8* buf = new uint8[size];
    if(compress((Bytef*)buf, &size, data, data_size) != Z_OK)
    {
        delete[] buf;
        return CPPBytes();
    }
    CPPBytes result;
    result.append(buf, size);
    delete[] buf;
    return result;
}

CPPBytes comex_zlib_decompress(const uint8* data, int data_size)
{
    if(data == NULL || data_size <= 0)
    {
        return CPPBytes();
    }
    z_stream ctx;
    ctx.zalloc = Z_NULL;
    ctx.zfree = Z_NULL;
    ctx.opaque = Z_NULL;
    ctx.avail_in = 0;
    ctx.next_in = Z_NULL;
    if(inflateInit(&ctx) != Z_OK)
    {
        return CPPBytes();
    }

    CPPBytes result;
    ctx.avail_in = data_size;
    ctx.next_in = (Bytef*)data;

    uint8_t buf[64] = {};
    while(true)
    {
        ctx.next_out = &buf[0];
        ctx.avail_out = sizeof(buf);

        int ret = inflate(&ctx, Z_NO_FLUSH);
        if(ret == Z_STREAM_END)
        {
            result.append(buf, sizeof(buf) - ctx.avail_out);
            if(ctx.avail_in == 0)
            {
                break;
            }
        }
        else if(ret != Z_OK)
        {
            break;
        }

        result.append(buf, sizeof(buf) - ctx.avail_out);
    }
    inflateEnd(&ctx);

    return result;
}

