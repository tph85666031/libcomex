#ifndef __COMEX_MAGIC_H__
#define __COMEX_MAGIC_H__

#include "com_base.h"

class COM_EXPORT FileMagic
{
public:
    FileMagic();
    virtual ~FileMagic();

    std::string getFileType(const char* file);
    std::string getContentType(const void* data, int data_size);
    std::string getContentType(const ComBytes& data);

private:
    void* ctx;
    ComBytes content;
};

#endif  /* __COMEX_MAGIC_H__ */

