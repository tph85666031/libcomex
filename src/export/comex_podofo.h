#ifndef __COMEX_PODOFO_H__
#define __COMEX_PODOFO_H__

#include "com_base.h"

class COM_EXPORT PdfExtrator
{
public:
    PdfExtrator();
    virtual ~PdfExtrator();

    void loadFromFile(const char* file);
    void loadFromMemory(const CPPBytes& content);

    std::string getText();
    std::vector<CPPBytes>& getImage();

    void extractText();
    void extractImage();
private:
    CPPBytes ppmToJpeg(int width, int height, const uint8* ppm, int ppm_size);
private:
    std::string file;
    CPPBytes content;
    std::string text;
    std::vector<CPPBytes> image;
};

#endif /* __COMEX_PODOFO_H__ */

