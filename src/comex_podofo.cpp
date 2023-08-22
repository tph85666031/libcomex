#include <stack>
#include "com_log.h"
#include "comex_podofo.h"

#include "jpeglib.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "podofo/podofo.h"
#pragma GCC diagnostic pop

using namespace PoDoFo;

PdfExtrator::PdfExtrator()
{
}

PdfExtrator::~PdfExtrator()
{
}

void PdfExtrator::loadFromFile(const char* file)
{
    if(file != NULL)
    {
        this->file = file;
    }
}

void PdfExtrator::loadFromMemory(const CPPBytes& content)
{
    this->content = content;
}

std::string PdfExtrator::getText()
{
    return text;
}

std::vector<CPPBytes>& PdfExtrator::getImage()
{
    return image;
}

CPPBytes PdfExtrator::ppmToJpeg(int width, int height, const uint8* ppm, int ppm_size)
{
    if(width <= 0 || height <= 0 || ppm == NULL || ppm_size <= 0)
    {
        return CPPBytes();
    }

    uint8* dst = NULL;
    size_t dst_size = 0;

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    memset(&cinfo, 0, sizeof(cinfo));
    memset(&jerr, 0, sizeof(jerr));

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, &dst, &dst_size);
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 100, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    JSAMPROW row_pointer[1] = {0};
    unsigned char bytes[width * 3] = {0};
    int ppm_pos = 0;
    while(cinfo.next_scanline < cinfo.image_height)
    {
        for(int i = 0;  i < width && ppm_pos < ppm_size; i += 3)
        {
            bytes[i] = ppm[ppm_pos++];
            bytes[i + 1] = ppm[ppm_pos++];
            bytes[i + 2] = ppm[ppm_pos++];
        }
        row_pointer[0] = bytes;
        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);

    CPPBytes result = CPPBytes(dst, dst_size);
    if(dst != NULL)
    {
        free(dst);
        dst = NULL;
    }

    jpeg_destroy_compress(&cinfo);
    return result;
}

void PdfExtrator::extractText()
{
    PdfMemDocument doc;
    try
    {
        if(file.empty())
        {
            doc.LoadFromBuffer(bufferview((const char*)content.getData(), content.getDataSize()));
        }
        else
        {
            doc.Load(file);
        }
    }
    catch(const PdfError& e)
    {
        LOG_E("failed:%s", e.what());
        return;
    }
    auto& pages = doc.GetPages();
    for(unsigned i = 0; i < pages.GetCount(); i++)
    {
        auto& page = pages.GetPageAt(i);

        std::vector<PdfTextEntry> entries;
        page.ExtractTextTo(entries);

        for(auto& entry : entries)
        {
            text.append(entry.Text + "\n");
        }
    }
    return;
}

void PdfExtrator::extractImage()
{
    PdfMemDocument doc;
    try
    {
        if(file.empty())
        {
            doc.LoadFromBuffer(bufferview((const char*)content.getData(), content.getDataSize()));
        }
        else
        {
            doc.Load(file);
        }
    }
    catch(const PdfError& e)
    {
        LOG_E("failed:%s", e.what());
        return;
    }

    for(PdfObject* obj : doc.GetObjects())
    {
        if(obj == NULL || obj->IsDictionary() == false)
        {
            continue;
        }
        PdfObject* typeObj = obj->GetDictionary().GetKey(PdfName::KeyType);
        PdfObject* subtypeObj = obj->GetDictionary().GetKey(PdfName::KeySubtype);
        if((typeObj && typeObj->IsName() && (typeObj->GetName() == "XObject")) ||
                (subtypeObj && subtypeObj->IsName() && (subtypeObj->GetName() == "Image")))
        {
            PdfObject* filter = obj->GetDictionary().GetKey(PdfName::KeyFilter);
            if(filter != NULL && filter->IsArray() && filter->GetArray().GetSize() == 1 &&
                    filter->GetArray()[0].IsName() && (filter->GetArray()[0].GetName() == "DCTDecode"))
            {
                filter = &filter->GetArray()[0];
            }

            if(filter != NULL && filter->IsName() && (filter->GetName() == "DCTDecode"))
            {
                const PdfMemoryObjectStream& memprovider = dynamic_cast<const PdfMemoryObjectStream&>(((const PdfObject&)(*obj)).GetStream()->GetProvider());
                auto& buffer = memprovider.GetBuffer();
                CPPBytes jpeg;
                jpeg.append((const uint8*)buffer.data(), buffer.size());
                image.push_back(jpeg);
            }
            else
            {
                int width = obj->GetDictionary().GetKey("Width")->GetNumber();
                int height = obj->GetDictionary().GetKey("Height")->GetNumber();
                auto buffer = obj->GetStream()->GetCopy();
                image.push_back(ppmToJpeg(width, height, (const uint8*)buffer.data(), buffer.size()));
            }
        }
    }
    return;
}

