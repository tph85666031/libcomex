#include <stack>
#include "com_log.h"
#include "com_file.h"
#include "comex_cairo_watermark.h"
#include "comex_podofo.h"

#include "jpeglib.h"
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "podofo/podofo.h"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

using namespace PoDoFo;

PdfReader::PdfReader()
{
    ctx = new PdfMemDocument();
}

PdfReader::~PdfReader()
{
    if(ctx != NULL)
    {
        delete(PdfMemDocument*)ctx;
        ctx = NULL;
    }
}

void PdfReader::loadFromFile(const char* file)
{
    if(ctx == NULL || file == NULL)
    {
        return;
    }
    try
    {
        ((PdfMemDocument*)ctx)->Load(file);
    }
    catch(const PdfError& e)
    {
        LOG_E("failed:%s", e.what());
        return;
    }
}

void PdfReader::loadFromMemory(const CPPBytes& content)
{
    loadFromMemory(content.getData(), content.getDataSize());
}

void PdfReader::loadFromMemory(const void* data, int data_size)
{
    if(ctx == NULL || data == NULL || data_size <= 0)
    {
        return;
    }
    try
    {
        ((PdfMemDocument*)ctx)->LoadFromBuffer(bufferview((const char*)data, data_size));
    }
    catch(const PdfError& e)
    {
        LOG_E("failed:%s", e.what());
        return;
    }
}

bool PdfReader::saveAs(const char* file)
{
    if(ctx == NULL || com_string_is_empty(file))
    {
        return false;
    }

    ((PdfMemDocument*)ctx)->Save(file);
    return true;
}

PdfExtrator::PdfExtrator()
{
}

PdfExtrator::~PdfExtrator()
{
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
    jpeg_mem_dest(&cinfo, &dst, (unsigned long*)&dst_size);
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 100, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    JSAMPROW row_pointer[1] = {0};
    unsigned char* bytes = new unsigned char[width * 3];
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
    delete[] bytes;

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
    auto& pages = ((PdfMemDocument*)ctx)->GetPages();
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
    for(PdfObject* obj : ((PdfMemDocument*)ctx)->GetObjects())
    {
        if(obj == NULL || obj->IsDictionary() == false)
        {
            continue;
        }
        PdfObject* type_obj = obj->GetDictionary().GetKey(PdfName::KeyType);
        PdfObject* subtype_obj = obj->GetDictionary().GetKey(PdfName::KeySubtype);
        if(type_obj == NULL || subtype_obj == NULL)
        {
            continue;
        }
        if((type_obj && type_obj->IsName() && (type_obj->GetName() == "XObject")) ||
                (subtype_obj && subtype_obj->IsName() && (subtype_obj->GetName() == "Image")))
        {
            PdfObject* filter = obj->GetDictionary().GetKey(PdfName::KeyFilter);
            if(filter == NULL)
            {
                continue;
            }

            if(filter->IsArray() && filter->GetArray().GetSize() == 1 &&
                    filter->GetArray()[0].IsName() && (filter->GetArray()[0].GetName() == "DCTDecode"))
            {
                filter = &filter->GetArray()[0];
            }

            if(filter->IsName() && (filter->GetName() == "DCTDecode"))
            {
                const PdfMemoryObjectStream& memprovider = dynamic_cast<const PdfMemoryObjectStream&>(((const PdfObject&)(*obj)).GetStream()->GetProvider());
                auto& buffer = memprovider.GetBuffer();
                CPPBytes jpeg;
                jpeg.append((const uint8*)buffer.data(), buffer.size());
                image.push_back(jpeg);
            }
            else
            {
                PdfObject* p_width = obj->GetDictionary().GetKey("Width");
                PdfObject* p_height = obj->GetDictionary().GetKey("Height");
                PdfObjectStream* p_stream = obj->GetStream();
                if(p_width != NULL && p_height != NULL && p_stream != NULL)
                {
                    try
                    {
                        int width = p_width->GetNumber();
                        int height = p_height->GetNumber();
#if 0
                        PdfObjectInputStream is = p_stream->GetInputStream(true);
                        charbuff buffer;
                        StringStreamDevice stream(buffer);
                        is.CopyTo(stream, true);
#else
                        auto buffer = p_stream->GetCopy();
#endif
                        image.push_back(ppmToJpeg(width, height, (const uint8*)buffer.data(), buffer.size()));
                    }
                    catch(PdfError& e)
                    {
                        LOG_E("%s", e.what());
                    }
                }
            }
        }
    }
    return;
}

PdfWatermark::PdfWatermark()
{
}

PdfWatermark::~PdfWatermark()
{
}

bool PdfWatermark::addWaterMark(const char* file_image_block, int space_x, int space_y)
{
    CPPBytes file_raw =  com_file_readall(file_image_block);
    auto& pages = ((PdfMemDocument*)ctx)->GetPages();
    std::unique_ptr<PdfImage> image = NULL;
    for(size_t i = 0; i < pages.GetCount(); i++)
    {
        PdfPage& page = pages.GetPageAt(i);
        Rect rect =  page.GetRect();
        if(image == NULL || (int)(image->GetRect().Width) != (int)(rect.Width) || (int)(image->GetRect().Height) != (int)(rect.Height))
        {
            CPPBytes result = WaterMark::ExpandWaterMark(file_raw, rect.Width, rect.Height, space_x, space_y);
            image = ((PdfMemDocument*)ctx)->CreateImage();
            image->LoadFromBuffer(bufferview((const char*)result.getData(), result.getDataSize()));
        }
        if(image == NULL)
        {
            return false;
        }
        PdfPainter painter;
        painter.SetCanvas(page);
        painter.DrawImage(*image, 0, 0);
    }
    return true;
}

std::vector<DarkMarkPos> PdfWatermark::addDarkMark(int pix_size)
{
    std::vector<DarkMarkPos> pos_list;
    auto& pages = ((PdfMemDocument*)ctx)->GetPages();
    for(size_t i = 0; i < pages.GetCount(); i++)
    {
        PdfPage& page = pages.GetPageAt(i);
        std::vector<PdfTextEntry> entries;
        page.ExtractTextTo(entries);
        if(entries.empty())
        {
            continue;
        }
        //检查文字类型，只在非符号文字下面做暗水印标记
        std::vector<PdfTextEntry> entries_refine;
        for(size_t n = 0; n < entries.size(); n++)
        {
            std::wstring text = com_wstring_from_utf8(CPPBytes(entries[n].Text.data(), entries[n].Text.length()));
            bool found = false;
            for(size_t j = 0; j < text.length(); j++)
            {
                if(std::iswalnum(text[j]) || (text[j] > 127 && std::iswgraph(text[j])))
                {
                    found = true;
                    break;
                }
            }
            if(found)
            {
                entries_refine.push_back(entries[n]);
            }
        }
        if(entries_refine.empty())
        {
            continue;
        }

        PdfTextEntry& entry = entries_refine[com_rand(0, entries_refine.size() - 1)];
        //一行文字中随机选择一个可见字符
        std::vector<int> cache_index;
        std::wstring text = com_wstring_from_utf8(CPPBytes(entry.Text.data(), entry.Text.length()));
        for(size_t j = 0; j < text.length(); j++)
        {
            if(std::iswalnum(text[j]) || text[j] > 127)
            {
                cache_index.push_back(j);
            }
        }
        int rand_index = cache_index[com_rand(0, cache_index.size() - 1)];
        double x = entry.X + entry.Length * rand_index / text.length();
        double y = entry.Y;
        LOG_D("%.0f:%.0f %s", x, y, entry.Text.c_str());
        PdfPainter painter;
        painter.SetCanvas(page);
        int type = com_rand(1, 4);
        //painter.DrawLine(x, y, x + entry.Length, y);
        if(type == 1)
        {
            //三角形
            painter.DrawLine(x, y - pix_size * 0.87 - 1, x + (double)pix_size / 2, y - 1);
            painter.DrawLine(x, y - pix_size * 0.87 - 1, x + pix_size, y - pix_size * 0.87 - 1);
            painter.DrawLine(x + (double)pix_size / 2, y - 1, x + pix_size, y - pix_size * 0.87 - 1);
        }
        else if(type == 2)
        {
            //圆形
            painter.DrawCircle(x + pix_size / 2, y - pix_size / 2 - 1, pix_size / 2);
        }
        else if(type == 3)
        {
            //正方形
            painter.DrawRectangle(x, y - pix_size - 1, pix_size, pix_size);
        }
        else if(type == 4)
        {
            //棱形
            painter.DrawLine(x, y - (double)pix_size / 2 - 1, x + (double)pix_size / 2, y - 1);
            painter.DrawLine(x, y - (double)pix_size / 2 - 1, x + (double)pix_size / 2, y - pix_size - 1);
            painter.DrawLine(x + (double)pix_size / 2, y - 1, x + pix_size, y - (double)pix_size / 2 - 1);
            painter.DrawLine(x + (double)pix_size / 2, y - pix_size - 1, x + pix_size, y - (double)pix_size / 2 - 1);
        }
        painter.FinishDrawing();
        DarkMarkPos pos;
        pos.page = i;
        pos.text = entry.Text;
        pos.x = x;
        pos.y = y;
        pos.type = type;
        pos_list.push_back(pos);
    }
    return pos_list;
}

