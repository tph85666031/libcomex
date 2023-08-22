#include "comex_podofo.h"
#include "com_log.h"

void comex_podofo_unit_test_suit(void** state)
{
    PdfExtrator extractor;
    extractor.loadFromFile("/data/3.pdf");
    extractor.extractText();
    extractor.extractImage();
    LOG_I("text=%s", extractor.getText().c_str());
}

