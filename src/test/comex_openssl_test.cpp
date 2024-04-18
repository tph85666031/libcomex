#include "comex_openssl.h"
#include "com_test.h"

void comex_openssl_aes_unit_test_suit(void** state)
{
    std::string key = "0123456789012345";
    std::string iv  = "0123456789012345";
    std::string key_xts = "01234567890123456789012345678901";
    std::string iv_xts  = "01234567890123456789012345678901";
    const char* file1 = "./aes1.dat";
    const char* file2 = "./aes2.dat";
    const char* file3 = "./aes3.dat";

    const char* modes[] = {"ECB", "CBC", "CFB1", "CFB8", "CFB128", "OFB", "CTR", "CCM", "GCM"};
    for(size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i++)
    {
        OpensslAES encrypt;
        if(com_string_equal(modes[i], "XTS"))
        {
            encrypt.setKey(key_xts);
            encrypt.setIV(iv_xts);
        }
        else
        {
            encrypt.setKey(key);
            encrypt.setIV(iv);
        }
        encrypt.setMode(modes[i]);
        CPPBytes bytes = encrypt.encrypt((uint8*)"12345678901234567", 17);
        LOG_D("AES[%s%zu]%s", modes[i], key.length() * 8, bytes.toHexString().c_str());

        OpensslAES decrypt;
        if(com_string_equal(modes[i], "XTS"))
        {
            decrypt.setKey(key_xts);
            decrypt.setIV(iv_xts);
        }
        else
        {
            decrypt.setKey(key);
            decrypt.setIV(iv);
        }
        decrypt.setTag(encrypt.getTag());
        decrypt.setMode(modes[i]);
        bytes = decrypt.decrypt(bytes.getData(), bytes.getDataSize());

        ASSERT_STR_EQUAL(bytes.toString().c_str(), "12345678901234567");

        com_file_writef(file1, "12345678901234567");
        OpensslAES aes1;
        aes1.setKey(key);
        aes1.setIV(iv);
        aes1.setMode(modes[i]);
        aes1.encryptFile(file1, file2);

        OpensslAES aes2;
        aes2.setKey(key);
        aes2.setIV(iv);
        aes2.setTag(aes1.getTag());
        aes2.setMode(modes[i]);
        aes2.decryptFile(file2, file3);

        CPPBytes b1 = com_file_readall(file1);
        CPPBytes b2 = com_file_readall(file1);

        ASSERT_TRUE(b1 == b2);
    }

    com_file_remove(file1);
    com_file_remove(file2);
    com_file_remove(file3);
}

void comex_openssl_des_unit_test_suit(void** state)
{
    std::string key = "01234567";
    std::string iv  = "01234567";
    const char* file1 = "./aes1.dat";
    const char* file2 = "./aes2.dat";
    const char* file3 = "./aes3.dat";

    const char* modes[] = {"ECB", "CBC", "CFB1", "CFB8", "CFB64"};
    for(size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i++)
    {
        OpensslDES encrypt;
        encrypt.setKey(key);
        encrypt.setIV(iv);
        encrypt.setMode(modes[i]);
        CPPBytes bytes = encrypt.encrypt((uint8*)"12345678901234567", 17);
        LOG_I("DES[%s]%s", modes[i], bytes.toHexString().c_str());

        OpensslDES decrypt;
        decrypt.setKey(key);
        decrypt.setIV(iv);
        decrypt.setMode(modes[i]);
        bytes = decrypt.decrypt(bytes.getData(), bytes.getDataSize());

        ASSERT_STR_EQUAL(bytes.toString().c_str(), "12345678901234567");

        com_file_writef(file1, "12345678901234567");
        OpensslDES des1;
        des1.setKey(key);
        des1.setIV(iv);
        des1.setMode(modes[i]);
        des1.encryptFile(file1, file2);

        OpensslDES des2;
        des2.setKey(key);
        des2.setIV(iv);
        des2.setMode(modes[i]);
        des2.decryptFile(file2, file3);

        CPPBytes b1 = com_file_readall(file1);
        CPPBytes b2 = com_file_readall(file1);

        ASSERT_TRUE(b1 == b2);
    }

    com_file_remove(file1);
    com_file_remove(file2);
    com_file_remove(file3);
}

void comex_openssl_des2_unit_test_suit(void** state)
{
    std::string key = "0123456789012345";
    std::string iv  = "0123456789012345";
    const char* file1 = "./aes1.dat";
    const char* file2 = "./aes2.dat";
    const char* file3 = "./aes3.dat";

    const char* modes[] = {"EDE-ECB", "EDE-CBC", "EDE-CFB64", "EDE-OFB"};
    for(size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i++)
    {
        Openssl2DES encrypt;
        encrypt.setKey(key);
        encrypt.setIV(iv);
        encrypt.setMode(modes[i]);
        CPPBytes bytes = encrypt.encrypt((uint8*)"12345678901234567", 17);
        LOG_D("2DES[%s]%s", modes[i], bytes.toHexString().c_str());

        Openssl2DES decrypt;
        decrypt.setKey(key);
        decrypt.setIV(iv);
        decrypt.setMode(modes[i]);
        bytes = decrypt.decrypt(bytes.getData(), bytes.getDataSize());

        ASSERT_STR_EQUAL(bytes.toString().c_str(), "12345678901234567");

        com_file_writef(file1, "12345678901234567");
        Openssl2DES des1;
        des1.setKey(key);
        des1.setIV(iv);
        des1.setMode(modes[i]);
        des1.encryptFile(file1, file2);

        Openssl2DES des2;
        des2.setKey(key);
        des2.setIV(iv);
        des2.setMode(modes[i]);
        des2.decryptFile(file2, file3);

        CPPBytes b1 = com_file_readall(file1);
        CPPBytes b2 = com_file_readall(file1);

        ASSERT_TRUE(b1 == b2);
    }

    com_file_remove(file1);
    com_file_remove(file2);
    com_file_remove(file3);
}

void comex_openssl_des3_unit_test_suit(void** state)
{
    std::string key = "012345678901234567890123";
    std::string iv  = "012345678901234567890124";
    const char* file1 = "./aes1.dat";
    const char* file2 = "./aes2.dat";
    const char* file3 = "./aes3.dat";

    const char* modes[] = {"EDE-ECB", "EDE-CBC", "EDE-CFB1", "EDE-CFB8", "EDE-CFB64", "EDE-OFB"};
    for(size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i++)
    {
        Openssl3DES encrypt;
        encrypt.setKey(key);
        encrypt.setIV(iv);
        encrypt.setMode(modes[i]);
        CPPBytes bytes = encrypt.encrypt((uint8*)"12345678901234567", 17);
        LOG_D("3DES[%s]%s", modes[i], bytes.toHexString().c_str());

        Openssl3DES decrypt;
        decrypt.setKey(key);
        decrypt.setIV(iv);
        decrypt.setMode(modes[i]);
        bytes = decrypt.decrypt(bytes.getData(), bytes.getDataSize());

        ASSERT_STR_EQUAL(bytes.toString().c_str(), "12345678901234567");

        com_file_writef(file1, "12345678901234567");
        Openssl3DES des1;
        des1.setKey(key);
        des1.setIV(iv);
        des1.setMode(modes[i]);
        des1.encryptFile(file1, file2);

        Openssl3DES des2;
        des2.setKey(key);
        des2.setIV(iv);
        des2.setMode(modes[i]);
        des2.decryptFile(file2, file3);

        CPPBytes b1 = com_file_readall(file1);
        CPPBytes b2 = com_file_readall(file1);

        ASSERT_TRUE(b1 == b2);
    }

    com_file_remove(file1);
    com_file_remove(file2);
    com_file_remove(file3);
}

static const char* cert_test = "-----BEGIN CERTIFICATE-----\n\
MIIFGzCCBAOgAwIBAgISAwDdVhTAY4XvdQD0CNqf5aVgMA0GCSqGSIb3DQEBCwUA\n\
MDIxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBFbmNyeXB0MQswCQYDVQQD\n\
EwJSMzAeFw0yMzAxMDkyMjA2MjZaFw0yMzA0MDkyMjA2MjVaMBUxEzARBgNVBAMT\n\
Cnpha2lyZC5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCrxxsM\n\
7cYB+Oqps88IF0+iy3w0xGYS5u/zmBd5yWXuZkwfmpJ9M+4H+i4VYve08x/VTy6x\n\
Z6hJQr/jzJq3MEbCaPUoqWRpb0xLZCTJ3O1Gn6Qfwu9vNtC8aSe44tYYcEAstPXu\n\
j/cNjG4Dkudd1j68u8lbKBCgWvY39eGeFSNybo5pAQmkjKTJ19sFAZBIS5AgjDh6\n\
CmB0eRgmMI5gCxe5JKCA3z8UANMJ5zRHNWN8VNKgneFX0csT0zwwJJeO6jQAn8xs\n\
DGr3VLxeYNxGMcIJ3tnD42MejxzFkJDo2oa+ffHDHxqGaZsL4LIMRwjIklkrZi/6\n\
oTihLxBl9pf9FoczAgMBAAGjggJGMIICQjAOBgNVHQ8BAf8EBAMCBaAwHQYDVR0l\n\
BBYwFAYIKwYBBQUHAwEGCCsGAQUFBwMCMAwGA1UdEwEB/wQCMAAwHQYDVR0OBBYE\n\
FGNOFYVWWqSUAsIWQqSll5o4AleXMB8GA1UdIwQYMBaAFBQusxe3WFbLrlAJQOYf\n\
r52LFMLGMFUGCCsGAQUFBwEBBEkwRzAhBggrBgEFBQcwAYYVaHR0cDovL3IzLm8u\n\
bGVuY3Iub3JnMCIGCCsGAQUFBzAChhZodHRwOi8vcjMuaS5sZW5jci5vcmcvMBUG\n\
A1UdEQQOMAyCCnpha2lyZC5jb20wTAYDVR0gBEUwQzAIBgZngQwBAgEwNwYLKwYB\n\
BAGC3xMBAQEwKDAmBggrBgEFBQcCARYaaHR0cDovL2Nwcy5sZXRzZW5jcnlwdC5v\n\
cmcwggEFBgorBgEEAdZ5AgQCBIH2BIHzAPEAdgB6MoxU2LcttiDqOOBSHumEFnAy\n\
E4VNO9IrwTpXo1LrUgAAAYWYyPsbAAAEAwBHMEUCIH25SgQnPaEJmjRKKG1y037c\n\
zswtnsux2s2qV1x1dn4MAiEApMPZGA5yP6qtvSSknr2wgeGWiHxVVz/Yky99pI9V\n\
c0AAdwC3Pvsk35xNunXyOcW6WPRsXfxCz3qfNcSeHQmBJe20mQAAAYWYyPz8AAAE\n\
AwBIMEYCIQCLrDmsdvTB7O2PMWQtkHtx6IlpVy4py8/JquGz5DLAwwIhAIMf2DVU\n\
RV1ipONEvFkAN+8+TGPMB2wZiNpId+00BKiMMA0GCSqGSIb3DQEBCwUAA4IBAQC2\n\
ZKTxFu4JCSGbnP/vBoGHpZbWYVEcdNGh/5QEi51V2kWF8BjWRNXVpIBgkciLvwrM\n\
qCMz4+PzqtiEpNjPnlBMXEULcNbp9IELoyFwhDS+YUWvRvpJny45yrLj7Z9WoCga\n\
2k/etYW29VfdK3y03puI7ob6P92lofGs6l+U86n7tp6LxVg+UIQI+okEw+0zaBwZ\n\
yCL85OcYJ8A47XPB9wHh4iUsBR0Y+z59l97qwRwBclzIZ1cbqbvF2pEdtkYsr10V\n\
mE1JWFAlg1KMnsevTrDZwhTRSwae6MF94nPoIMtrTS8twaJCrDKkBLveC5PH42id\n\
Y5BeUeqqGJJKAfLrXCgQ\n\
-----END CERTIFICATE-----";

void comex_openssl_unit_test_suit(void** state)
{
    OpensslMD5 md5_openssl;
    CPPMD5 md5_cpp;
    for(int i = 0; i < 10; i++)
    {
        uint8 buf[1024];
        for(size_t j = 0; j < sizeof(buf); j++)
        {
            buf[j] = (uint8)com_rand(0, 9);
        }
        md5_openssl.append(buf, sizeof(buf));
        md5_cpp.append(buf, sizeof(buf));
        ASSERT_STR_EQUAL(md5_cpp.finish().toHexString().c_str(), md5_openssl.finish().toHexString().c_str());
    }

    OpensslSHA1 sha1;
    sha1.append((uint8*)"0123456789", strlen("0123456789"));
    ASSERT_STR_EQUAL(sha1.finish().toHexString().c_str(), "87acec17cd9dcd20a716cc2cf67417b71c8a7016");

    OpensslSHA256 sha256;
    sha256.append((uint8*)"0123456789", strlen("0123456789"));
    ASSERT_STR_EQUAL(sha256.finish().toHexString().c_str(), "84d89877f0d4041efb6bf91a16f0248f2fd573e6af05c19f96bedb9f882f7882");

    OpensslSHA512 sha512;
    sha512.append((uint8*)"0123456789", strlen("0123456789"));
    ASSERT_STR_EQUAL(sha512.finish().toHexString().c_str(), "bb96c2fc40d2d54617d6f276febe571f623a8dadf0b734855299b0e107fda32cf6b69f2da32b36445d73690b93cbd0f7bfc20e0f7f28553d2a4428f23b716e90");

    OpensslHMAC hmac;
    hmac.setType("sha256");
    hmac.setKey("123456");
    hmac.append((uint8*)"0123456789", strlen("0123456789"));
    ASSERT_STR_EQUAL(hmac.finish().toHexString().c_str(), "47b9be2425afe3157b036154d4c7bb497735c4ee5515aed9ef550a2c5d13aaa4");
    CPPBytes hmac_hash = OpensslHMAC::Digest("sha256", "0123456789", strlen("0123456789"), "123456", 6);
    ASSERT_STR_EQUAL(hmac_hash.toHexString().c_str(), "47b9be2425afe3157b036154d4c7bb497735c4ee5515aed9ef550a2c5d13aaa4");

    std::string public_key;
    std::string private_key;
    OpensslRSA::GenerateKey(1024, public_key, private_key, "123456");
    ASSERT_FALSE(public_key.empty());
    ASSERT_FALSE(private_key.empty());

    OpensslRSA rsa;
    rsa.setPaddingPKCS1();
    rsa.setPublicKey(public_key.c_str());
    rsa.setPrivateKey(private_key.c_str(), "123456");
    private_key.clear();

    CPPBytes bytes = rsa.encryptWithPublicKey((uint8*)"123456", 6);
    bytes = rsa.decryptWithPrivateKey(bytes.getData(), bytes.getDataSize());
    ASSERT_STR_EQUAL("123456", bytes.toString().c_str());
#if 0
    bytes = rsa.encryptWithPrivateKey((uint8*)"123456", 6);
    bytes = rsa.decryptWithPublicKey(bytes.getData(), bytes.getDataSize());
    ASSERT_STR_EQUAL("123456", bytes.toString().c_str());
#endif
    std::string hmac_sha256 = OpensslHMAC::Digest("SHA256", "123", 3, "kkkkk").toHexString();
    ASSERT_STR_EQUAL(hmac_sha256.c_str(), "93e4aaa0c8f96fa9eb18c96a29c7557830809af1294b88411d929fc4d35a4292");

    OpensslCert cert;
    cert.loadFromMem(cert_test);

    LOG_I("issuer=%s", cert.issuer.c_str());
    LOG_I("subject=%s", cert.subject.c_str());
    LOG_I("sn=%s", cert.sn.c_str());
    LOG_I("signature_algorithm=%s", cert.signature_algorithm.c_str());
    LOG_I("valid_begin=%llu", cert.valid_begin);
    LOG_I("valid_end=%llu", cert.valid_end);


    //CERT CREATE
    Message cert_params;
    cert_params.set("subject", "CN=EndpointFVS,O=Skyguard,C=CN,L=BJ,S=BJ");
    cert_params.set("file_crt", "./test.crt");
    cert_params.set("file_p12", "./test.p12");
    cert_params.set("password_private_key", "skyguard_endpoint_eps");
    cert_params.set("dns", "www.baidu.com,www.163.com");
    cert_params.set("ip", "192.168.0.11,192.168.0.12");
    ASSERT_TRUE(OpensslCert::CreateCert(cert_params));
    //com_file_remove("./test.p12");
    //com_file_remove("./test.crt");
}

