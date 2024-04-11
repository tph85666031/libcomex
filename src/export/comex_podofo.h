#ifndef __COMEX_PODOFO_H__
#define __COMEX_PODOFO_H__

#include "com_base.h"

class COM_EXPORT PdfReader
{
public:
    PdfReader();
    virtual ~PdfReader();
    void loadFromFile(const char* file);
    void loadFromMemory(const CPPBytes& content);
    void loadFromMemory(const void* data, int data_size);

    bool saveAs(const char* file);
protected:
    void* ctx;
};

class COM_EXPORT PdfExtrator : public PdfReader
{
public:
    PdfExtrator();
    virtual ~PdfExtrator();

    std::string getText();
    std::vector<CPPBytes>& getImage();

    void extractText();
    void extractImage();
private:
    CPPBytes ppmToJpeg(int width, int height, const uint8* ppm, int ppm_size);
private:
    std::string text;
    std::vector<CPPBytes> image;
};

class DarkMarkPos
{
public:
    std::string text;
    int page;
    int x;
    int y;
    int type;
};

class COM_EXPORT PdfWatermark : public PdfReader
{
public:
    PdfWatermark();
    virtual ~PdfWatermark();
    bool addWaterMark(const char* file_image_block, int space_x = 100, int space_y = 100);
    std::vector<DarkMarkPos> addDarkMark(int pix_size);
private:
    std::string file;
    CPPBytes content;
};

#endif /* __COMEX_PODOFO_H__ */

