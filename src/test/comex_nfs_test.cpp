#include "comex_nfs.h"
#include "com_test.h"
#include "com_file.h"
#include "com_log.h"

void comex_nfs_unit_test_suit(void** state)
{
    ComexNfs nfs;
    nfs.setHost("172.22.72.2").setShareName("nfs");
    nfs.setUID(0).setGID(0);
    std::map<std::string, int> list = nfs.ls(PATH_TO_LOCAL(".\\").c_str());
    for(auto it = list.begin(); it != list.end(); it++)
    {
        LOG_I("%d:%s", it->second, it->first.c_str());
    }

    com_file_remove(PATH_TO_LOCAL("./__nfs_test__.txt").c_str());
    ASSERT_TRUE(com_file_writef(PATH_TO_LOCAL("./__nfs_test__.txt").c_str(), "nfs test data") > 0);
    int file_size = com_file_size(PATH_TO_LOCAL("./__nfs_test__.txt").c_str());
    ASSERT_INT_EQUAL(file_size, sizeof("nfs test data") - 1);

    ASSERT_TRUE(nfs.put(PATH_TO_LOCAL("./__nfs_test__.txt").c_str()));
    ASSERT_INT_EQUAL(nfs.getFileSize("__nfs_test__.txt"), file_size);

    ASSERT_TRUE(nfs.putAs(PATH_TO_LOCAL("./__nfs_test__.txt").c_str(), PATH_TO_LOCAL("s1/s2/s3/__nfs_test__.txt").c_str()));
    ASSERT_INT_EQUAL(nfs.getFileSize(PATH_TO_LOCAL("s1/s2/s3/__nfs_test__.txt").c_str()), file_size);

    ASSERT_TRUE(nfs.get("__nfs_test__.txt"));
    ASSERT_INT_EQUAL(com_file_size(PATH_TO_LOCAL("./__nfs_test__.txt").c_str()), file_size);

    ASSERT_TRUE(nfs.getAs("__nfs_test__.txt", PATH_TO_LOCAL("./xxx/s1/s2/nfs.txt").c_str()));
    ASSERT_INT_EQUAL(com_file_size(PATH_TO_LOCAL("./xxx/s1/s2/nfs.txt").c_str()), file_size);
    com_dir_remove(PATH_TO_LOCAL("./xxx/s1").c_str());

    ComBytes bytes = nfs.getBytes("__nfs_test__.txt");
    ASSERT_INT_EQUAL(bytes.getDataSize(), file_size);

    ASSERT_INT_EQUAL(nfs.getFileType("__sss__/__not_exist__"), FILE_TYPE_NOT_EXIST);

    com_file_remove(PATH_TO_LOCAL("./__nfs_test__.txt").c_str());

    ASSERT_TRUE(nfs.remove("__nfs_test__.txt"));
    ASSERT_TRUE(nfs.remove(PATH_TO_LOCAL("s1/s2/s3/__nfs_test__.txt").c_str()));
    ASSERT_TRUE(nfs.rmdir("s1"));
}

