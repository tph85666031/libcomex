#ifndef __COMEX_PODOFO_H__
#define __COMEX_PODOFO_H__

#include "com_base.h"

class COM_EXPORT PdfTextExtrator
{
public:
    PdfTextExtrator();
    virtual ~PdfTextExtrator();

    std::string getText();
    void extractText(const char* file);
private:
    std::string text;
};

#endif /* __COMEX_PODOFO_H__ */

