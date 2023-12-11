#include "comex_cairo_watermark.h"

void comex_cairo_watermark_unit_test_suit(void** state)
{
    WaterMark wm;
    wm.setType(WATER_MARK_TYPE_TEXT);
    wm.setText("adfsb bwhvb\nwovenbo");
    wm.createWatermark().toFile("/data/1.png");
}

