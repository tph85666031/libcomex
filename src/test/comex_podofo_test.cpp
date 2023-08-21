#include "comex_podofo.h"
#include "com_log.h"

void comex_podofo_unit_test_suit(void** state)
{
    PdfTextExtrator extractor;
    extractor.extractText("./3.pdf");
    //LOG_I("text=%s",extractor.getText().c_str());
}

