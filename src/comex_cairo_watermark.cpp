#include <cairo/cairo.h>
#include <fontconfig/fontconfig.h>

#define M_PI       3.14159265358979323846   // pi

/*
    此文件如果静态编译需依赖如下库(c/c++基础库除外)
      libcairo.a
      libfontconfig.a
      libfreetype.a
      libxml2.a
      libbz2.a
      libexpat.a
      libharfbuzz.a
      libbrotlidec.a
      libbrotlienc.a
      libbrotlicommon.a
      libpng.a
      libz.a
      libpixman-1.a
      libqrencode.a
*/
#include <math.h>
#include <qrencode.h>
#include <vector>
#include "com_base.h"
#include "com_file.h"
#include "com_log.h"
#include "comex_cairo_watermark.h"

int dot_s[3][3] =
{
    {1, 1, 1},
    {0, 0, 1},
    {0, 0, 0}
};

int dot_0[3][3] =
{
    {0, 1, 0},
    {0, 1, 0},
    {0, 0, 1}
};

int dot_1[3][3] =
{
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, 1}
};

int dot_2[3][3] =
{
    {0, 0, 0},
    {0, 1, 1},
    {1, 0, 0}
};

int dot_3[3][3] =
{
    {1, 0, 0},
    {0, 0, 0},
    {1, 0, 1}
};

int dot_4[3][3] =
{
    {0, 0, 1},
    {0, 1, 0},
    {0, 1, 0}
};

int dot_5[3][3] =
{
    {0, 0, 0},
    {1, 1, 1},
    {0, 0, 0}
};

int dot_6[3][3] =
{
    {1, 0, 0},
    {0, 1, 1},
    {0, 0, 0}
};

int dot_7[3][3] =
{
    {0, 0, 1},
    {0, 1, 0},
    {1, 0, 0}
};

int dot_8[3][3] =
{
    {0, 1, 0},
    {0, 1, 0},
    {1, 0, 0}
};

int dot_9[3][3] =
{
    {1, 0, 1},
    {0, 0, 0},
    {0, 0, 1}
};

int dot_a[3][3] =
{
    {1, 0, 0},
    {0, 1, 0},
    {0, 1, 0}
};

int dot_b[3][3] =
{
    {0, 0, 1},
    {1, 1, 0},
    {0, 0, 0}
};

int dot_c[3][3] =
{
    {0, 0, 0},
    {1, 1, 0},
    {0, 0, 1}
};

int dot_d[3][3] =
{
    {0, 1, 0},
    {0, 1, 0},
    {0, 1, 0}
};

int dot_e[3][3] =
{
    {1, 0, 1},
    {0, 0, 0},
    {1, 0, 0}
};

int dot_f[3][3] =
{
    {0, 0, 1},
    {0, 0, 0},
    {1, 0, 1}
};

static cairo_status_t cairo_png_write_func(void* ctx, const unsigned char* data, unsigned int length)
{
    if(ctx == NULL || data == NULL || length <= 0)
    {
        return CAIRO_STATUS_WRITE_ERROR;
    }

    ComBytes* bytes = (ComBytes*)ctx;
    bytes->append(data, length);
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t cairo_png_read_func(void* ctx, unsigned char* data, unsigned int length)
{
    if(ctx == NULL || data == NULL || length <= 0)
    {
        return CAIRO_STATUS_READ_ERROR;
    }

    ComBytes* bytes = (ComBytes*)ctx;
    if(bytes->getDataSize() < (int)length)
    {
        return CAIRO_STATUS_READ_ERROR;
    }

    memcpy(data, bytes->getData(), length);
    bytes->removeHead(length);
    return CAIRO_STATUS_SUCCESS;
}

WaterMark::WaterMark()
{
    font_name = detectFontNameFromLang();
}

WaterMark::~WaterMark()
{
}

int WaterMark::getType()
{
    return this->type;
}

double WaterMark::getFontSize()
{
    return this->font_size;
}

std::string WaterMark::getFontName()
{
    return this->font_name;
}

unsigned int WaterMark::getColor()
{
    return this->color;
}

unsigned int WaterMark::getBackgroundColor()
{
    return this->color_background;
}

int WaterMark::getPixSize()
{
    return this->pix_size;
}

int WaterMark::getAngle()
{
    return this->angle;
}

int WaterMark::getWidth()
{
    return this->width;
}

int WaterMark::getSpaceX()
{
    return this->space_x;
}

int WaterMark::getSpaceY()
{
    return this->space_y;
}

double WaterMark::getAlpha()
{
    return this->alpha;
}

double WaterMark::getBackgroundAlpha()
{
    return this->alpha_backgound;
}

std::string WaterMark::getText()
{
    std::string text_tmp = this->text;
    if(isTimestampRequired())
    {
        text_tmp += com_time_to_string(com_time_rtc_s() + com_timezone_get_s());
    }
    return text_tmp;
}

unsigned char WaterMark::getBinarize()
{
    return this->binarize;
}

double WaterMark::getDPI()
{
    return this->dpi;
}

std::string WaterMark::detectFontNameFromLang(const char* lang)
{
    std::string lang_str;
    if(lang == NULL)
    {
        lang_str = com_user_get_language();
    }
    else
    {
        lang_str = lang;
    }
    LOG_I("lang_str=%s", lang_str.c_str());
    com_string_replace(lang_str, "_", "-");
    lang_str = lang_str.substr(0, lang_str.find("."));

    FcInit();
    std::string pattern_filter = com_string_format(":lang=%s", lang_str.c_str());
    FcConfig* config = FcInitLoadConfigAndFonts();
    if(config == NULL)
    {
        return std::string();
    }
    FcPattern* pat = FcNameParse((FcChar8*)pattern_filter.c_str());
    if(pat == NULL)
    {
        FcConfigDestroy(config);
        return std::string();
    }
    FcObjectSet* os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_LANG, FC_FILE, (char*) 0);
    if(os == NULL)
    {
        FcPatternDestroy(pat);
        FcConfigDestroy(config);
        return std::string();
    }
    FcFontSet* fs = FcFontList(config, pat, os);
    if(fs == NULL)
    {
        FcPatternDestroy(pat);
        FcConfigDestroy(config);
        FcObjectSetDestroy(os);
        return std::string();
    }
    LOG_D("total matching fonts: %d,lang=%s", fs->nfont, lang_str.c_str());
    std::string font_name;
    for(int i = 0; i < fs->nfont; i++)
    {
        FcPattern* font = fs->fonts[i];
        FcChar8* file = NULL;
        FcChar8* style = NULL;
        FcChar8* family = NULL;
        if(FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch
                && FcPatternGetString(font, FC_STYLE, 0, &style) == FcResultMatch
                && FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch)
        {
            LOG_D("Filename: %s (family %s, style %s)", (char*)file, (char*)family, (char*)style);
            font_name = (char*)family;
            break;
        }
    }
    FcPatternDestroy(pat);
    FcConfigDestroy(config);
    FcObjectSetDestroy(os);
    FcFontSetDestroy(fs);
    return font_name;
}

WaterMark& WaterMark::setType(int type)
{
    if(type == WATER_MARK_TYPE_TEXT
            || type == WATER_MARK_TYPE_QRCODE
            || type == WATER_MARK_TYPE_DOT
            || type == WATER_MARK_TYPE_NONE)
    {
        this->type = type;
    }
    return *this;
}

WaterMark& WaterMark::setFontSize(double size)
{
    if(size >= 0)
    {
        this->font_size = size;
    }
    return *this;
}

WaterMark& WaterMark::setFontName(const char* name)
{
    if(name != NULL)
    {
        this->font_name = name;
    }
    return *this;
}

WaterMark& WaterMark::setColor(uint32 color)
{
    this->color = color;
    return *this;
}

WaterMark& WaterMark::setBackgroundColor(uint32 color)
{
    this->color_background = color;
    return *this;
}

WaterMark& WaterMark::setPixSize(int pix_size)
{
    if(pix_size >= 0)
    {
        this->pix_size = pix_size;
    }
    return *this;
}

WaterMark& WaterMark::setAngle(int angle)
{
    if(angle >= 0)
    {
        this->angle = angle;
    }
    return *this;
}

WaterMark& WaterMark::setWidth(int width)
{
    if(width >= 0)
    {
        this->width = width;
    }
    return *this;
}

WaterMark& WaterMark::setSpace(int space_x, int space_y)
{
    if(space_x >= 0)
    {
        this->space_x = space_x;
    }
    if(space_y >= 0)
    {
        this->space_y = space_y;
    }
    return *this;
}

WaterMark& WaterMark::setAlpha(double alpha)
{
    this->alpha = alpha;
    return *this;
}

WaterMark& WaterMark::setBackgroundAlpha(double alpha)
{
    this->alpha_backgound = alpha;
    return *this;
}

WaterMark& WaterMark::setText(const char* text)
{
    if(text != NULL)
    {
        this->text = text;
    }
    return *this;
}

WaterMark& WaterMark::setText(const std::string& text)
{
    this->text = text;
    return *this;
}

WaterMark& WaterMark::setBinarize(uint8 binarize)
{
    this->binarize = binarize;
    return *this;
}

WaterMark& WaterMark::setDPI(double dpi)
{
    if(dpi > 0)
    {
        this->dpi = dpi;
    }
    return *this;
}

WaterMark& WaterMark::setTimestampRequired(bool required)
{
    this->timestamp_required = required;
    return *this;
}

WaterMark& WaterMark::setQRCodeCombineBackground(bool combine)
{
    this->qrcode_combinebackground = combine;
    return *this;
}

bool WaterMark::isTimestampRequired()
{
    return this->timestamp_required;
}

bool WaterMark::createWatermark(const char* file)
{
    return createWatermark().toFile(file);
}

ComBytes WaterMark::createWatermark()
{
    if(type == WATER_MARK_TYPE_TEXT)
    {
        return createWatermarkAsText();
    }
    else if(type == WATER_MARK_TYPE_QRCODE)
    {
        return createWatermarkAsQRCode();
    }
    else if(type == WATER_MARK_TYPE_DOT)
    {
        return createWatermarkAsDot();
    }
    else
    {
        LOG_E("watermark type not support, type=%d", type);
        return ComBytes();
    }
}

bool WaterMark::createWatermarkAsQRCode(const char* file)
{
    return createWatermarkAsQRCode().toFile(file);
}

bool WaterMark::createWatermarkAsText(const char* file)
{
    return createWatermarkAsText().toFile(file);
}

bool WaterMark::createWatermarkAsDot(const char* file)
{
    return createWatermarkAsDot().toFile(file);
}

ComBytes WaterMark::createWatermarkAsQRCode()
{
    QRcode* qr = QRcode_encodeString(getText().c_str(), 1, QR_ECLEVEL_M, QR_MODE_8, 1);
    if(qr == NULL)
    {
        return ComBytes();
    }

    int block_width = width + space_x;
    int block_height = width + space_y;
    cairo_surface_t* surface_block = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, block_width, block_height); //创建一个图像外观
    cairo_t* cr_block = cairo_create(surface_block);

    float R_background = (float)(color_background >> 24 & 0xFF) / 255;
    float G_background = (float)(color_background >> 16 & 0xFF) / 255;
    float B_background = (float)(color_background >> 8 & 0xFF) / 255;

    float R = (float)(color >> 24 & 0xFF) / 255;
    float G = (float)(color >> 16 & 0xFF) / 255;
    float B = (float)(color >> 8 & 0xFF) / 255;

    if(alpha_backgound > 0)//绘制背景色
    {
        cairo_set_operator(cr_block, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba(cr_block, R_background, G_background, B_background, alpha_backgound);
        cairo_paint(cr_block);
    }

    //以中心点旋转
    cairo_translate(cr_block, block_width / 2, block_height / 2);
    cairo_rotate(cr_block, 2 * M_PI * angle / 360);
    cairo_translate(cr_block, -block_width / 2, -block_height / 2);

    int pix_size = width / qr->width;
    if(pix_size <= 0)
    {
        pix_size = 1;
    }
    cairo_set_operator(cr_block, CAIRO_OPERATOR_SOURCE);
    cairo_move_to(cr_block, space_x / 2, space_y / 2);
    cairo_set_line_width(cr_block, 0.1);
    for(int y = 0; y < qr->width; y++)
    {
        for(int x = 0; x < qr->width; x++)
        {
            uint8 bit = qr->data[y * qr->width + x];
            if(bit & 0x01)
            {
                cairo_set_source_rgba(cr_block, R, G, B, alpha); /* 设置颜色 */
                cairo_rectangle(cr_block,  space_x / 2 + x * pix_size,  space_y / 2 + y * pix_size, pix_size, pix_size);
            }
            else
            {
                if(qrcode_combinebackground)
                {
                    cairo_set_source_rgba(cr_block, R_background, G_background, B_background, alpha_backgound);
                    cairo_rectangle(cr_block,  space_x / 2 + x * pix_size,  space_y / 2 + y * pix_size, pix_size, pix_size);
                }
                else
                {
                    cairo_set_source_rgba(cr_block, 1 - R, 1 - G, 1 - B, alpha);
                    cairo_rectangle(cr_block,  space_x / 2 + x * pix_size,  space_y / 2 + y * pix_size, pix_size, pix_size);
                }
            }

            cairo_fill(cr_block);
            cairo_move_to(cr_block, space_x / 2 + (x + 1) * pix_size, space_y / 2 + y * pix_size);
        }
    }

    //二值化
    if(getBinarize() > 0)
    {
        uint8* data = cairo_image_surface_get_data(surface_block);
        int stride = cairo_image_surface_get_stride(surface_block);
        for(int y = 0; y < block_height; y++)
        {
            for(int x = 0; x < stride; x = x + 4)
            {
                data[y * stride + x + 0] = (data[y * stride + x + 0] >= getBinarize() * alpha) ? 255 * alpha : 0;
                data[y * stride + x + 1] = (data[y * stride + x + 1] >= getBinarize() * alpha) ? 255 * alpha : 0;
                data[y * stride + x + 2] = (data[y * stride + x + 2] >= getBinarize() * alpha) ? 255 * alpha : 0;
            }
        }
    }

    cairo_destroy(cr_block);
    ComBytes data;
    cairo_surface_write_to_png_stream(surface_block, cairo_png_write_func, &data);
    cairo_surface_destroy(surface_block);
    QRcode_free(qr);

    return data;
}

ComBytes WaterMark::createWatermarkAsText()
{
    std::vector<std::string> vals = com_string_split(getText().c_str(), "\n");
    double font_size_px = (font_size * dpi / 72) + 1;
    int block_width  = width + space_x;
    int block_height = width + space_y;//高度调整为固定font_height * vals.size() + space_y;
    cairo_surface_t* surface_block = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, block_width, block_height); //创建一个图像外观
    cairo_t* cr_block = cairo_create(surface_block);
    cairo_select_font_face(cr_block, font_name.c_str(), CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL); //设置字体
    cairo_set_font_size(cr_block, font_size_px);//设置字体大小
    float R = 0;
    float G = 0;
    float B = 0;
    if(alpha_backgound > 0)//绘制背景色
    {
        cairo_set_operator(cr_block, CAIRO_OPERATOR_SOURCE);
        R = (float)(color_background >> 24 & 0xFF) / 255;
        G = (float)(color_background >> 16 & 0xFF) / 255;
        B = (float)(color_background >> 8 & 0xFF) / 255;
        cairo_set_source_rgba(cr_block, R, G, B, alpha_backgound);
        cairo_paint(cr_block);
    }

    R = (float)(color >> 24 & 0xFF) / 255;
    G = (float)(color >> 16 & 0xFF) / 255;
    B = (float)(color >> 8 & 0xFF) / 255;

    cairo_set_operator(cr_block, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr_block, R, G, B, alpha); /* 设置颜色 */

    //DEBUG:绘制实际水印边界
    //cairo_rectangle(cr_block,  space_x / 2,  space_y / 2, width, width);
    //cairo_stroke_preserve(cr_block);

    //设置文本为垂直居中
    float y_offset = (block_height - vals.size() * font_size_px) / 2;
    if(y_offset < 0)
    {
        y_offset = 0;
    }

    //绘制文本
    for(size_t i = 0; i < vals.size(); i++)
    {
        cairo_move_to(cr_block, space_x / 2, y_offset + (i + 1) * font_size_px);
        cairo_show_text(cr_block, vals[i].c_str());
    }

    //二值化
    if(getBinarize() > 0)
    {
        uint8* data = cairo_image_surface_get_data(surface_block);
        int stride = cairo_image_surface_get_stride(surface_block);
        for(int y = 0; y < block_height; y++)
        {
            for(int x = 0; x < stride; x = x + 4)
            {
                data[y * stride + x + 0] = (data[y * stride + x + 0] >= getBinarize() * alpha) ? 255 * alpha : 0;
                data[y * stride + x + 1] = (data[y * stride + x + 1] >= getBinarize() * alpha) ? 255 * alpha : 0;
                data[y * stride + x + 2] = (data[y * stride + x + 2] >= getBinarize() * alpha) ? 255 * alpha : 0;
            }
        }
    }

    //创建带旋转的新图层
    cairo_surface_t* surface_block_final = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, block_width, block_height); //创建一个图像外观
    cairo_t* cr_block_final = cairo_create(surface_block_final);
    cairo_translate(cr_block_final, block_width / 2, block_height / 2);
    cairo_rotate(cr_block_final, 2 * M_PI * angle / 360);
    cairo_translate(cr_block_final, -block_width / 2, -block_height / 2);

    //将surface_block绘制到新图层实现旋转功能
    cairo_set_source_surface(cr_block_final, surface_block,  0, 0);
    cairo_paint(cr_block_final);

    //将新图层保存为png
    ComBytes data;
    cairo_surface_write_to_png_stream(surface_block_final, cairo_png_write_func, &data);

    //资源回收
    cairo_destroy(cr_block);
    cairo_destroy(cr_block_final);
    cairo_surface_destroy(surface_block);
    cairo_surface_destroy(surface_block_final);

    return data;
}

ComBytes WaterMark::createWatermarkAsDot()
{
    std::string text_tmp = getText();
    if(text_tmp.length() < 9 || text_tmp.at(0) != 's')
    {
        LOG_E("text incorrect:%s", text_tmp.c_str());
        return ComBytes();
    }
    com_string_to_lower(text_tmp);

    // 图像尺寸
    int block_width = width + space_x;
    int block_height = width + space_y;
    float r = (float)pix_size / 2;
    // 创建画布
    cairo_surface_t* surface_block = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, block_width, block_height); //创建一个图像外观
    cairo_t* cr_block = cairo_create(surface_block);

    float R = 0;
    float G = 0;
    float B = 0;

    if(alpha_backgound > 0)//绘制背景色
    {
        cairo_set_operator(cr_block, CAIRO_OPERATOR_SOURCE);
        R = (float)(color_background >> 24 & 0xFF) / 255;
        G = (float)(color_background >> 16 & 0xFF) / 255;
        B = (float)(color_background >> 8 & 0xFF) / 255;
        cairo_set_source_rgba(cr_block, R, G, B, alpha_backgound);
        cairo_paint(cr_block);
    }

    R = (float)(color >> 24 & 0xFF) / 255;
    G = (float)(color >> 16 & 0xFF) / 255;
    B = (float)(color >> 8 & 0xFF) / 255;

    //以中心点旋转
    cairo_translate(cr_block, block_width / 2, block_height / 2);
    cairo_rotate(cr_block, 2 * M_PI * angle / 360);
    cairo_translate(cr_block, -block_width / 2, -block_height / 2);

    if(pix_size <= 0)
    {
        pix_size = 1;
    }
    cairo_set_line_width(cr_block, 0.1);
    cairo_set_operator(cr_block, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr_block, R, G, B, alpha); /* 设置颜色 */
    float space_dot = ((float)width - 11.0f * pix_size) / 8.0f;//累计11个点(包含3x3区域间隔空点)8个空位间距

    int dot_9x9[9][9];
    memset(dot_9x9, 0, sizeof(dot_9x9));
    for(int i = 0; i < 9; i++)
    {
        auto p = dot_s;
        switch(text_tmp.at(i))
        {
            case 's':
                p = dot_s;
                break;
            case '0':
                p = dot_0;
                break;
            case '1':
                p = dot_1;
                break;
            case '2':
                p = dot_2;
                break;
            case '3':
                p = dot_3;
                break;
            case '4':
                p = dot_4;
                break;
            case '5':
                p = dot_5;
                break;
            case '6':
                p = dot_6;
                break;
            case '7':
                p = dot_7;
                break;
            case '8':
                p = dot_8;
                break;
            case '9':
                p = dot_9;
                break;
            case 'a':
                p = dot_a;
                break;
            case 'b':
                p = dot_b;
                break;
            case 'c':
                p = dot_c;
                break;
            case 'd':
                p = dot_d;
                break;
            case 'e':
                p = dot_e;
                break;
            case 'f':
                p = dot_f;
                break;
            default:
                LOG_E("text incorrect:text[i]=%c", text_tmp.at(i));
                return false;
        }
        //将3x3点阵按序拷贝到9x9中指定的3x3区域
        int x_start = (i / 3) * 3;
        int y_start = (i % 3) * 3;
        dot_9x9[x_start + 0][y_start + 0] = p[0][0];
        dot_9x9[x_start + 0][y_start + 1] = p[0][1];
        dot_9x9[x_start + 0][y_start + 2] = p[0][2];

        dot_9x9[x_start + 1][y_start + 0] = p[1][0];
        dot_9x9[x_start + 1][y_start + 1] = p[1][1];
        dot_9x9[x_start + 1][y_start + 2] = p[1][2];

        dot_9x9[x_start + 2][y_start + 0] = p[2][0];
        dot_9x9[x_start + 2][y_start + 1] = p[2][1];
        dot_9x9[x_start + 2][y_start + 2] = p[2][2];
    }

    float pos_y = (float)space_y / 2 + r; //pos_x,pos_y是圆心位置，需要加半径偏移防止图像绘制在边界外
    for(int i = 0; i < 9; i++)
    {
        float pos_x = (float)space_x / 2 + r; //pos_x,pos_y是圆心位置，需要加半径偏移防止图像绘制在边界外
        for(int j = 0; j < 9; j++)
        {
            int val = dot_9x9[i][j];
            if(val & 0x01)
            {
                cairo_arc(cr_block, pos_x, pos_y, r, 0, 2 * M_PI);
                cairo_fill(cr_block);
            }
            pos_x += (space_dot + pix_size);
            if(j == 2 || j == 5)
            {
                pos_x += pix_size;//添加3x3分隔间隔
            }
        }
        pos_y += (space_dot + pix_size);
        if(i == 2 || i == 5)
        {
            pos_y += pix_size;//添加3x3分隔间隔
        }
    }

    //二值化
    if(getBinarize() > 0)
    {
        uint8* data = cairo_image_surface_get_data(surface_block);
        int stride = cairo_image_surface_get_stride(surface_block);
        for(int y = 0; y < block_height; y++)
        {
            for(int x = 0; x < stride; x = x + 4)
            {
                data[y * stride + x + 0] = (data[y * stride + x + 0] >= getBinarize() * alpha) ? 255 * alpha : 0;
                data[y * stride + x + 1] = (data[y * stride + x + 1] >= getBinarize() * alpha) ? 255 * alpha : 0;
                data[y * stride + x + 2] = (data[y * stride + x + 2] >= getBinarize() * alpha) ? 255 * alpha : 0;
            }
        }
    }

    cairo_destroy(cr_block);
    ComBytes data;
    cairo_surface_write_to_png_stream(surface_block, cairo_png_write_func, &data);
    cairo_surface_destroy(surface_block);

    return data;
}

ComBytes WaterMark::ExpandWaterMark(const ComBytes& block, int width, int height, int space_x, int space_y)
{
    if(width <= 0 || height <= 0)
    {
        return ComBytes();
    }
    ComBytes block_copy = block;
    cairo_surface_t* surface_block = cairo_image_surface_create_from_png_stream(cairo_png_read_func, &block_copy);
    if(space_x > 0 || space_y > 0)//添加边距
    {
        int block_width = cairo_image_surface_get_width(surface_block);
        int block_height = cairo_image_surface_get_height(surface_block);
        if(block_width <= 0 || block_height <= 0)
        {
            cairo_surface_destroy(surface_block);
            return ComBytes();
        }
        cairo_surface_t* surface_block_margin = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, block_width + space_x, block_height + space_y);
        cairo_t* cr_block_margin = cairo_create(surface_block_margin);
        cairo_set_source_surface(cr_block_margin, surface_block, space_x / 2, space_y / 2);
        cairo_paint(cr_block_margin);
        cairo_destroy(cr_block_margin);
        cairo_surface_destroy(surface_block);
        surface_block = surface_block_margin;
    }

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create(surface);
    cairo_pattern_t* pattern = cairo_pattern_create_for_surface(surface_block);
    cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT); //平铺
    cairo_set_source(cr, pattern);
    cairo_paint(cr);

    cairo_destroy(cr);
    cairo_pattern_destroy(pattern);
    cairo_surface_destroy(surface_block);
    ComBytes data;
    cairo_surface_write_to_png_stream(surface, cairo_png_write_func, &data);
    cairo_surface_destroy(surface);
    return data;
}

bool WaterMark::ExpandWaterMark(const char* file, const char* file_block,
                                int width, int height, int space_x, int space_y)
{
    return ExpandWaterMark(com_file_readall(file_block), width, height, space_x, space_y).toFile(file);
}

bool WaterMark::ExpandWaterMark(const char* file, const ComBytes& block,
                                int width, int height, int space_x, int space_y)
{
    return ExpandWaterMark(block, width, height, space_x, space_y).toFile(file);
}

int WaterMark::GetPNGAlphaValueMax(const char* file)
{
    if(file == NULL)
    {
        LOG_E("arg incorrect");
        return -1;
    }
    cairo_surface_t* surface = cairo_image_surface_create_from_png(file);
    uint8* data = cairo_image_surface_get_data(surface);
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    int stride = cairo_image_surface_get_stride(surface);

    if(width * 4 != stride || cairo_image_surface_get_format(surface) != CAIRO_FORMAT_ARGB32)
    {
        LOG_E("not png file");
        return -1;
    }

    int value = 0;
    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < stride; x = x + 4)
        {
            uint8 alpha = data[y * stride + x + 3];
            if(value < alpha)
            {
                value = alpha;
            }
        }
    }

    cairo_surface_destroy(surface);
    return value;
}

bool WaterMark::SetPNGAlphaValue(const char* file_to, const char* file_from, int value,
                                 uint8_t threshold_begin, uint8_t threshold_end)
{
    if(file_to == NULL || file_from == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    value = value < 0 ? 0 : value;
    value = value > 255 ? 255 : value;
    cairo_surface_t* surface = cairo_image_surface_create_from_png(file_from);
    uint8* data = cairo_image_surface_get_data(surface);
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    int stride = cairo_image_surface_get_stride(surface);

    if(width * 4 != stride || cairo_image_surface_get_format(surface) != CAIRO_FORMAT_ARGB32)
    {
        LOG_E("not png file");
        return false;
    }

    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < stride; x = x + 4)
        {
            uint8 alpha = data[y * stride + x + 3];
            if(alpha >= threshold_begin && alpha <= threshold_end)
            {
                data[y * stride + x + 3] = value;
            }
        }
    }

    cairo_surface_write_to_png(surface, file_to);
    cairo_surface_destroy(surface);
    return true;
}

bool WaterMark::SetPNGBrighValue(const char* file_to, const char* file_from, double brightness)
{
    if(file_to == NULL || file_from == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    if(brightness < 0)
    {
        brightness = 0;
    }
    if(brightness > 1)
    {
        brightness = 1;
    }
    cairo_surface_t* surface = cairo_image_surface_create_from_png(file_from);
    uint8* data = cairo_image_surface_get_data(surface);
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    int stride = cairo_image_surface_get_stride(surface);

    if(width * 4 != stride || cairo_image_surface_get_format(surface) != CAIRO_FORMAT_ARGB32)
    {
        LOG_E("not png file");
        return false;
    }

    LOG_I("xxx=%f", brightness);
    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < stride; x = x + 4)
        {
            COLOR_RGB rgb = {(double)data[y * stride + x + 0] / 255, (double)data[y * stride + x + 1] / 255, (double)data[y * stride + x + 2] / 255};
            COLOR_HSV hsv = RGB2HSV(rgb);
            hsv.v = brightness;
            rgb = HSV2RGB(hsv);
            data[y * stride + x + 0] = rgb.r * 255;
            data[y * stride + x + 1] = rgb.g * 255;
            data[y * stride + x + 2] = rgb.b * 255;
        }
    }

    cairo_surface_write_to_png(surface, file_to);
    cairo_surface_destroy(surface);
    return true;
}

COLOR_HSV WaterMark::RGB2HSV(COLOR_RGB& rgb)
{
    COLOR_HSV hsv;
    double      min, max, delta;

    min = rgb.r < rgb.g ? rgb.r : rgb.g;
    min = min  < rgb.b ? min  : rgb.b;

    max = rgb.r > rgb.g ? rgb.r : rgb.g;
    max = max  > rgb.b ? max  : rgb.b;

    hsv.v = max;                                // v
    delta = max - min;
    if(delta < 0.00001)
    {
        hsv.s = 0;
        hsv.h = 0; // undefined, maybe nan?
        return hsv;
    }
    if(max > 0.0)     // NOTE: if Max is == 0, this divide would cause a crash
    {
        hsv.s = (delta / max);                  // s
    }
    else
    {
        // if max is 0, then r = g = b = 0
        // s = 0, h is undefined
        hsv.s = 0.0;
        hsv.h = 0;                            // its now undefined
        return hsv;
    }
    if(rgb.r >= max)                              // > is bogus, just keeps compilor happy
    {
        hsv.h = (rgb.g - rgb.b) / delta;          // between yellow & magenta
    }
    else if(rgb.g >= max)
    {
        hsv.h = 2.0 + (rgb.b - rgb.r) / delta;    // between cyan & yellow
    }
    else
    {
        hsv.h = 4.0 + (rgb.r - rgb.g) / delta;    // between magenta & cyan
    }

    hsv.h *= 60.0;                              // degrees
    if(hsv.h < 0.0)
    {
        hsv.h += 360.0;
    }

    return hsv;
}

COLOR_RGB WaterMark::HSV2RGB(COLOR_HSV& hsv)
{
    COLOR_RGB rgb;
    double hh, p, q, t, ff;

    if(hsv.s <= 0.0)         // < is bogus, just shuts up warnings
    {
        rgb.r = hsv.v;
        rgb.g = hsv.v;
        rgb.b = hsv.v;
        return rgb;
    }
    hh = hsv.h;
    if(hh >= 360.0)
    {
        hh = 0.0;
    }
    hh /= 60.0;
    long i = (long)hh;
    ff = hh - i;
    p = hsv.v * (1.0 - hsv.s);
    q = hsv.v * (1.0 - (hsv.s * ff));
    t = hsv.v * (1.0 - (hsv.s * (1.0 - ff)));

    switch(i)
    {
        case 0:
            rgb.r = hsv.v;
            rgb.g = t;
            rgb.b = p;
            break;
        case 1:
            rgb.r = q;
            rgb.g = hsv.v;
            rgb.b = p;
            break;
        case 2:
            rgb.r = p;
            rgb.g = hsv.v;
            rgb.b = t;
            break;

        case 3:
            rgb.r = p;
            rgb.g = q;
            rgb.b = hsv.v;
            break;
        case 4:
            rgb.r = t;
            rgb.g = p;
            rgb.b = hsv.v;
            break;
        case 5:
        default:
            rgb.r = hsv.v;
            rgb.g = p;
            rgb.b = q;
            break;
    }
    return rgb;
}

