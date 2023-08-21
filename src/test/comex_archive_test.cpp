#include "comex.h"

void comex_archive_unit_test_suit(void** state)
{
    com_dir_create(PATH_TO_LOCAL("./archive_test").c_str());
    com_file_writef(PATH_TO_LOCAL("./archive_test/1.txt").c_str(), "A123456");
    com_file_writef(PATH_TO_LOCAL("./archive_test/2.txt").c_str(), "B123456");
    com_file_writef(PATH_TO_LOCAL("./archive_test/3.txt").c_str(), "C123456");

    ArchiveZip z1;
    z1.open(PATH_TO_LOCAL("./z1.zip").c_str(), false, true);
    ASSERT_TRUE(z1.addToFile(PATH_TO_LOCAL("a.txt").c_str(), "adsfg", 5));
    ASSERT_TRUE(z1.addToFile(PATH_TO_LOCAL("archive_test/a.txt").c_str(), "12345", 5));
    ASSERT_TRUE(z1.addDirectory(PATH_TO_LOCAL("archive_test").c_str(), PATH_TO_LOCAL("./archive_test").c_str(), false));
    ASSERT_INT_EQUAL(z1.getFileSize(PATH_TO_LOCAL("a.txt").c_str()), 5);
    ASSERT_INT_EQUAL(z1.getFileSize(PATH_TO_LOCAL("archive_test/a.txt").c_str()), 5);
    ASSERT_INT_EQUAL(z1.getFileSize(PATH_TO_LOCAL("archive_test/1.txt").c_str()), 7);
    ASSERT_INT_EQUAL(z1.getFileSize(PATH_TO_LOCAL("archive_test/2.txt").c_str()), 7);
    ASSERT_INT_EQUAL(z1.getFileSize(PATH_TO_LOCAL("archive_test/3.txt").c_str()), 7);
    z1.close();

    ArchiveZip z2;
    z2.open(PATH_TO_LOCAL("./z1.zip").c_str());
    ASSERT_TRUE(z2.extractTo(PATH_TO_LOCAL("./dir_z1").c_str()));
    ASSERT_INT_EQUAL(z2.getFileSize(PATH_TO_LOCAL("a.txt").c_str()), 5);
    ASSERT_INT_EQUAL(z2.getFileSize(PATH_TO_LOCAL("archive_test/a.txt").c_str()), 5);
    ASSERT_INT_EQUAL(z2.getFileSize(PATH_TO_LOCAL("archive_test/1.txt").c_str()), 7);
    ASSERT_INT_EQUAL(z2.getFileSize(PATH_TO_LOCAL("archive_test/2.txt").c_str()), 7);
    ASSERT_INT_EQUAL(z2.getFileSize(PATH_TO_LOCAL("archive_test/3.txt").c_str()), 7);
    z2.close();
    com_file_remove(PATH_TO_LOCAL("./z1.zip").c_str());
    com_dir_remove(PATH_TO_LOCAL("./dir_z1").c_str());

    ArchiveWriter w0(PATH_TO_LOCAL("./t1.tar.gz").c_str());
    w0.addDirectory("archive_test", PATH_TO_LOCAL("./archive_test").c_str(), false);
    w0.close();

    ArchiveReader r1(PATH_TO_LOCAL("./t1.tar.gz").c_str());
    ASSERT_INT_EQUAL(r1.getFileSize(PATH_TO_LOCAL("./archive_test/1.txt").c_str()), 7);
    ASSERT_INT_EQUAL(r1.getFileSize(PATH_TO_LOCAL("./archive_test/2.txt").c_str()), 7);
    ASSERT_INT_EQUAL(r1.getFileSize(PATH_TO_LOCAL("./archive_test/3.txt").c_str()), 7);

    CPPBytes b = r1.read(PATH_TO_LOCAL("./archive_test/1.txt").c_str());
    ASSERT_STR_EQUAL(b.toString().c_str(), "A123456");
    r1.readTo(PATH_TO_LOCAL("archive_test/1.txt").c_str(), "./__t1__.txt");
    ASSERT_STR_EQUAL(com_file_readall(PATH_TO_LOCAL("./__t1__.txt").c_str()).toString().c_str(), "A123456");
    com_file_remove(PATH_TO_LOCAL("./__t1__.txt").c_str());

    ArchiveWriter w1(PATH_TO_LOCAL("./w1.tar.xz").c_str());
    w1.addFile("1.txt", PATH_TO_LOCAL("archive_test/1.txt").c_str());
    w1.addDirectory("archive_test", PATH_TO_LOCAL("./archive_test").c_str(), false);
    w1.close();

    ArchiveReader r2(PATH_TO_LOCAL("./w1.tar.xz").c_str());
    ASSERT_INT_EQUAL(r2.getFileSize(PATH_TO_LOCAL("archive_test/1.txt").c_str()), 7);
    ASSERT_INT_EQUAL(r2.getFileSize(PATH_TO_LOCAL("archive_test/2.txt").c_str()), 7);
    ASSERT_INT_EQUAL(r2.getFileSize(PATH_TO_LOCAL("archive_test/3.txt").c_str()), 7);

    com_file_remove(PATH_TO_LOCAL("./t1.tar.gz").c_str());
    com_file_remove(PATH_TO_LOCAL("./w1.tar.xz").c_str());
    com_dir_remove(PATH_TO_LOCAL("./archive_test").c_str());
}

