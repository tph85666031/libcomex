#include "com_log.h"
#include "com_file.h"
#include "comex_cairo_watermark.h"

void comex_cairo_watermark_unit_test_suit(void** state)
{
    WaterMark wm;
    wm.setType(WATER_MARK_TYPE_TEXT);
    wm.setText("sadfsbbwhvb\nwovenbo中文");
    wm.createWatermark().toFile("./1.png");

    wm.setType(WATER_MARK_TYPE_QRCODE);
    wm.createWatermark().toFile("./2.png");

    wm.setType(WATER_MARK_TYPE_DOT);
    wm.setText("s12345678");
    wm.createWatermark().toFile("./3.png");

    com_file_remove("./1.png");
    com_file_remove("./2.png");
    com_file_remove("./3.png");
}

