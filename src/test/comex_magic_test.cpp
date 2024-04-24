#include "com_log.h"
#include "com_file.h"
#include "comex_magic.h"

void comex_magic_unit_test_suit(void** state)
{
    FileMagic m;
	
    LOG_I("type=%s", m.getFileType("./1.txt").c_str());
    LOG_I("type=%s", m.getFileType("./1.tar.xz").c_str());
    LOG_I("type=%s", m.getFileType("./1.doc").c_str());
    LOG_I("type=%s", m.getFileType("./1.docx").c_str());
    LOG_I("type=%s", m.getFileType("./x.pdf").c_str());
    LOG_I("type=%s", m.getFileType("./image.png").c_str());
	LOG_I("type=%s", m.getFileType("./test.crt").c_str());
	LOG_I("type=%s", m.getFileType("./test.p12").c_str());

    LOG_I("type=%s", m.getContentType(com_file_readall("./image.png")).c_str());
}

