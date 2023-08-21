#ifndef __COMEX_ZLIB_H__
#define __COMEX_ZLIB_H__

#include "com_base.h"

COM_EXPORT CPPBytes comex_zlib_compress(const uint8* data, int data_size);
COM_EXPORT CPPBytes comex_zlib_decompress(const uint8* data, int data_size);

#endif /* __COMEX_ZLIB_H__ */

