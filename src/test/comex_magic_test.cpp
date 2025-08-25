#include "com_log.h"
#include "com_file.h"
#include "comex_magic.h"

void comex_magic_unit_test_suit(void** state)
{
    FileMagic m;
    std::map<std::string, int> list;
    com_dir_list("/data", list, false);
    for(auto it = list.begin(); it != list.end(); it++)
    {
        LOG_I("type=%s:%s", it->first.c_str(), m.getFileType(it->first.c_str()).c_str());
    }
}

