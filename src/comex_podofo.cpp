#include <stack>
#include "com_log.h"
#include "comex_podofo.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "podofo/podofo.h"
#pragma GCC diagnostic pop

using namespace PoDoFo;

PdfTextExtrator::PdfTextExtrator()
{
}

PdfTextExtrator::~PdfTextExtrator()
{
}

std::string PdfTextExtrator::getText()
{
    return text;
}

void PdfTextExtrator::extractText(const char* file)
{
    PdfMemDocument doc;
    doc.Load(file);
    auto& pages = doc.GetPages();
    for(unsigned i = 0; i < pages.GetCount(); i++)
    {
        auto& page = pages.GetPageAt(i);

        std::vector<PdfTextEntry> entries;
        page.ExtractTextTo(entries);

        for(auto& entry : entries)
        {
            //printf("(%.3f,%.3f) %s \n", entry.X, entry.Y, entry.Text.data());
            text.append(entry.Text + "\n");
        }
    }
    return;
}

