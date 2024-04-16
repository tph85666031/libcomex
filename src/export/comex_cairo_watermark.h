#ifndef __COMEX_CAIRO_WATERMARK_H__
#define __COMEX_CAIRO_WATERMARK_H__

#include "com_base.h"

#define WATER_MARK_TYPE_NONE     0
#define WATER_MARK_TYPE_TEXT     1
#define WATER_MARK_TYPE_QRCODE   2
#define WATER_MARK_TYPE_DOT      3

typedef struct
{
    double r; //[0-1]
    double g; //[0-1]
    double b; //[0-1]
} COLOR_RGB;

typedef struct
{
    double h; //[0-359]
    double s; //[0-1]
    double v; //[0-1]
} COLOR_HSV;

class WaterMark
{
public:
    WaterMark();
    ~WaterMark();
    int getType();
    double getFontSize();
    std::string getFontName();
    unsigned int getColor();
    unsigned int getBackgroundColor();
    int getPixSize();
    int getAngle();
    int getWidth();
    int getSpaceX();
    int getSpaceY();
    double getAlpha();
    double getBackgroundAlpha();
    std::string getText();
    unsigned char getBinarize();
    double getDPI();
    bool isTimestampRequired();

    WaterMark& setType(int type);//水印类型
    WaterMark& setFontSize(double size);//字体大小
    WaterMark& setFontName(const char* name);//字体名字
    WaterMark& setColor(unsigned int color);//颜色
    WaterMark& setBackgroundColor(unsigned int color);//背景颜色
    WaterMark& setPixSize(int pix_size);//单颗像素大小(二维码和DOT有效)
    WaterMark& setAngle(int angle);//旋转角度
    WaterMark& setWidth(int width);//适应宽度
    WaterMark& setSpace(int space_x, int space_y);//设置间距
    WaterMark& setAlpha(double alpha);//透明度
    WaterMark& setBackgroundAlpha(double alpha);//背景透明度
    //如果水印类型为WATER_MARK_TYPE_DOT，则text必须以s开始且总长度不低于9字节
    WaterMark& setText(const char* text);//水印文本
    WaterMark& setText(const std::string& text);//水印文本
    WaterMark& setBinarize(unsigned char binarize = 100); //水印像素二值化
    WaterMark& setTimestampRequired(bool required);//水印是否叠加当前时间戳
    WaterMark& setDPI(double dpi);//设置屏幕DPI
    WaterMark& setQRCodeCombineBackground(bool combine);//二维码0点位置使用背景配色

    bool createWatermark(const char* file);
    CPPBytes createWatermark();
    //平铺水印图片到指定分辨率
    static CPPBytes ExpandWaterMark(const CPPBytes& block, int width, int height, int space_x = 0, int space_y = 0);
    static bool ExpandWaterMark(const char* file, const char* file_block,
                                int width, int height, int space_x = 0, int space_y = 0);
    static bool ExpandWaterMark(const char* file, const CPPBytes& block,
                                int width, int height, int space_x = 0, int space_y = 0);

    //设置PNG透明度
    static bool SetPNGAlphaValue(const char* file_to, const char* file_from, int value,
                                 unsigned char threshold_begin = 1, unsigned char threshold_end = 255);
    //设置图片亮度,取值范围为[0,1]
    static bool SetPNGBrighValue(const char* file_to, const char* file_from, double bright);
    static int GetPNGAlphaValueMax(const char* file);
private:
    bool createWatermarkAsQRCode(const char* file);
    bool createWatermarkAsText(const char* file);
    bool createWatermarkAsDot(const char* file);

    CPPBytes createWatermarkAsQRCode();
    CPPBytes createWatermarkAsText();
    CPPBytes createWatermarkAsDot();

    std::string detectFontNameFromLang(const char* lang = NULL);
    
    static COLOR_HSV RGB2HSV(COLOR_RGB& rgb);
    static COLOR_RGB HSV2RGB(COLOR_HSV& hsv);
private:
    std::string text;
    int type = WATER_MARK_TYPE_NONE;
    double font_size = 12.0f;
    std::string font_name;
    unsigned int color = 0xC0C0C000;//默认25%灰
    unsigned int color_background = 0xFFFFFF00;//纯白
    int width = 100;
    int space_x = 0;
    int space_y = 0;
    int angle = 0;
    int pix_size = 5;
    double dpi = 96;//默认DPI
    double alpha = 1.0f;//默认不透明
    double alpha_backgound = 0.0f;//默认全透明
    unsigned char binarize = 0;//默认不做二值化
    bool timestamp_required = false;
    bool qrcode_combinebackground = false;
};

#endif /* __COMEX_CAIRO_WATERMARK_H__ */

