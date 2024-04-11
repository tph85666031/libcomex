#include "comex_podofo.h"
#include "com_log.h"

void comex_podofo_unit_test_suit(void** state)
{
    PdfExtrator extractor;
    extractor.loadFromFile("/data/4.pdf");
    extractor.extractText();
    extractor.extractImage();
    std::vector<CPPBytes>& images = extractor.getImage();
    for(size_t i = 0; i < images.size(); i++)
    {
        //images[i].toFile(com_string_format("/data/img/%hu.jpg", i).c_str());
    }
    //LOG_I("text=%s", extractor.getText().c_str());

    PdfWatermark w;
    w.loadFromFile("/data/2.pdf");
    w.addWaterMark("/data/1.png", 100, 100);


    w.addDarkMark(5);
    w.saveAs("/data/x.pdf");
}

