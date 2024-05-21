#include "comex.h"
#include "com_test.h"

void comex_smb_unit_test_suit(void** state)
{
    ComexSmb smb_test;
    smb_test.setShareUrlPath("smb://root@192.168.0.11/data");
    smb_test.setPassword("root");
    std::map<std::string, int> r = smb_test.ls("");
    ASSERT_TRUE(r.size() > 0);

    ComexSmb smb;
    smb.setHost("192.168.0.11").setUsername("root").setPassword("root").setShareName("data");

    //prepare environment
    smb.remove("__smb_test__.txt");
    smb.rmdir("s1");
    com_file_remove(PATH_TO_LOCAL("./__smb_test__.txt").c_str());

    //start unit test
    ASSERT_TRUE(com_file_writef(PATH_TO_LOCAL("./__smb_test__.txt").c_str(), "smb test data") > 0);
    int file_size = com_file_size(PATH_TO_LOCAL("./__smb_test__.txt").c_str());
    ASSERT_INT_EQUAL(file_size, sizeof("smb test data") - 1);

    ASSERT_TRUE(smb.put(PATH_TO_LOCAL("./__smb_test__.txt").c_str()));
    ASSERT_INT_EQUAL(smb.getFileSize("__smb_test__.txt"), file_size);

    ASSERT_TRUE(smb.putAs(PATH_TO_LOCAL("./__smb_test__.txt").c_str(), PATH_TO_LOCAL("s1/s2/s3/__smb_test__.txt").c_str()));
    ASSERT_INT_EQUAL(smb.getFileSize(PATH_TO_LOCAL("s1/s2/s3/__smb_test__.txt").c_str()), file_size);

    ASSERT_TRUE(smb.get("__smb_test__.txt"));
    ASSERT_INT_EQUAL(com_file_size(PATH_TO_LOCAL("./__smb_test__.txt").c_str()), file_size);

    ASSERT_TRUE(smb.getAs("__smb_test__.txt", PATH_TO_LOCAL("./xxx/s1/s2/smb.txt").c_str()));
    ASSERT_INT_EQUAL(com_file_size(PATH_TO_LOCAL("./xxx/s1/s2/smb.txt").c_str()), file_size);
    com_dir_remove(PATH_TO_LOCAL("./xxx/s1").c_str());

    ComBytes bytes = smb.getBytes("__smb_test__.txt");
    ASSERT_INT_EQUAL(bytes.getDataSize(), file_size);

    ASSERT_INT_EQUAL(smb.getFileType(PATH_TO_LOCAL("__sss__/__not_exist__").c_str()), FILE_TYPE_NOT_EXIST);

    com_file_remove(PATH_TO_LOCAL("./__smb_test__.txt").c_str());

    ASSERT_TRUE(smb.remove("__smb_test__.txt"));
    ASSERT_TRUE(smb.remove(PATH_TO_LOCAL("s1/s2/s3/__smb_test__.txt").c_str()));
    ASSERT_TRUE(smb.rmdir("s1"));
}

