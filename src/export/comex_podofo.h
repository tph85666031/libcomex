#ifndef __COMEX_PODOFO_H__
#define __COMEX_PODOFO_H__

#include "com_base.h"

class COM_EXPORT ComexPdfReader
{
public:
    ComexPdfReader();
    virtual ~ComexPdfReader();
    void loadFromFile(const char* file);
    void loadFromMemory(const ComBytes& content);
    void loadFromMemory(const void* data, int data_size);

    bool saveAs(const char* file);
protected:
    void* ctx;
};

class COM_EXPORT ComexPdfExtrator : public ComexPdfReader
{
public:
    ComexPdfExtrator();
    virtual ~ComexPdfExtrator();

    std::string getText();
    std::vector<ComBytes>& getImage();

    void extractText();
    void extractImage();
private:
    ComBytes ppmToJpeg(int width, int height, const uint8* ppm, int ppm_size);
private:
    std::string text;
    std::vector<ComBytes> image;
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

class COM_EXPORT ComexPdfWatermark : public ComexPdfReader
{
public:
    ComexPdfWatermark();
    virtual ~ComexPdfWatermark();
    bool addWaterMark(const char* file_image_block, int space_x = 100, int space_y = 100);
    std::vector<DarkMarkPos> addDarkMark(int pix_size);
private:
    std::string file;
    ComBytes content;
};

#endif /* __COMEX_PODOFO_H__ */

