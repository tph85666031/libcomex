#include "com_log.h"
#include "com_file.h"
#include "comex_magic.h"

void comex_magic_unit_test_suit(void** state)
{
    FileMagic m;

    LOG_I("type=%s", m.getFileType("/data/1.txt").c_str());
    LOG_I("type=%s", m.getFileType("/data/1.tar.xz").c_str());
    LOG_I("type=%s", m.getFileType("/data/1.doc").c_str());
    LOG_I("type=%s", m.getFileType("/data/1.docx").c_str());
    LOG_I("type=%s", m.getFileType("/data/x.pdf").c_str());
    LOG_I("type=%s", m.getFileType("/data/image.png").c_str());

    LOG_I("type=%s", m.getContentType(com_file_readall("/data/image.png")).c_str());
}

