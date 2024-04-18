#ifndef __COMEX_OPENSSL_H__
#define __COMEX_OPENSSL_H__

#include "com.h"

#define OPENSSL_DES_BLOCK_SIZE   8
#define OPENSSL_AES_TAG_SIZE     16

class COM_EXPORT OpensslHash
{
public:
    OpensslHash(const char* type);
    virtual ~OpensslHash();

    bool append(const void* data, int data_size);
    CPPBytes finish();

    static CPPBytes Digest(const char* type, const void* data, int data_size);
private:
    void* ctx = NULL;
    const void* digest = NULL;
};

class COM_EXPORT OpensslMD5 : public OpensslHash
{
public:
    OpensslMD5() : OpensslHash("MD5") {};
};

class COM_EXPORT OpensslSHA1 : public OpensslHash
{
public:
    OpensslSHA1() : OpensslHash("SHA1") {};
};

class COM_EXPORT OpensslSHA224 : public OpensslHash
{
public:
    OpensslSHA224() : OpensslHash("SHA224") {};
};

class COM_EXPORT OpensslSHA256 : public OpensslHash
{
public:
    OpensslSHA256() : OpensslHash("SHA256") {};
};

class COM_EXPORT OpensslSHA512 : public OpensslHash
{
public:
    OpensslSHA512() : OpensslHash("SHA512") {};
};

class COM_EXPORT OpensslSM3 : public OpensslHash
{
public:
    OpensslSM3() : OpensslHash("SM3") {};
};

class COM_EXPORT OpensslHMAC
{
public:
    OpensslHMAC();
    virtual ~OpensslHMAC();

    bool setType(const char* type);
    bool setKey(const char* key);
    bool setKey(const uint8* key, int key_size);
    bool append(const void* data, int data_size);
    CPPBytes finish();
public:
    static CPPBytes Digest(const char* type, const void* data, int data_size, const void* key = NULL, int key_size = 0);
private:
    CPPBytes key;
    std::string type;
    void* mac = NULL;
    void* ctx = NULL;
};

class COM_EXPORT OpensslCrypto
{
public:
    OpensslCrypto();
    virtual ~OpensslCrypto();

    bool setMode(const char* mode);

    bool setIV(const char* iv);
    bool setIV(const std::string& iv);
    bool setIV(uint8* iv_data, int iv_size);

    bool setKey(const char* key);
    bool setKey(const std::string& key);
    bool setKey(uint8* key, int key_size);

    bool setTag(const char* tag);
    bool setTag(const std::string& tag);
    bool setTag(uint8* tag, int tag_size);
    bool setTag(const CPPBytes& tag);

    CPPBytes getTag();

    CPPBytes encrypt(const uint8* data, int data_size);
    CPPBytes decrypt(const uint8* data, int data_size);

    bool encryptFile(const char* file_src, const char* file_dst, bool attach_tag = false);
    bool decryptFile(const char* file_src, const char* file_dst);

    bool encryptBegin();
    bool encryptAppend(CPPBytes& result, const uint8* data, int data_size);
    bool encryptEnd(CPPBytes& result);

    bool decryptBegin();
    bool decryptAppend(CPPBytes& result, const uint8* data, int data_size);
    bool decryptEnd(CPPBytes& result);
protected:
    virtual const void* getCipher() = 0;
protected:
    std::string engine_mode;
    std::string padd_mode;

    void* ctx = NULL;
    CPPBytes key;
    CPPBytes iv;
    CPPBytes tag;
    uint8 buf[4096];
    int block_size = 0;
};

/*
    name    key           iv           tag
    ECB     16B,24B,32B   N/A          N/A
    CBC     16B,24B,32B   16B,24B,32B  N/A
    CFB1    16B,24B,32B   16B,24B,32B  N/A
    CFB8    16B,24B,32B   16B,24B,32B  N/A
    CFB128  16B,24B,32B   16B,24B,32B  N/A
    OFB     16B,24B,32B   16B,24B,32B  N/A
    CTR     16B,24B,32B   16B,24B,32B  N/A
    CCM     16B,24B,32B   16B,24B,32B  nB
    GCM     16B,24B,32B   16B,24B,32B  nB
    XTS     32B,64B       32B,64B      N/A
*/
class COM_EXPORT OpensslAES : public OpensslCrypto
{
public:
    OpensslAES();
    virtual ~OpensslAES();
private:
    const void* getCipher();
};

/*
    name    key     iv
    ECB     8B      N/A
    CBC     8B      8B
    CFB1    8B      8B
    CFB8    8B      8B
    CFB64   8B      8B
*/
class COM_EXPORT OpensslDES : public OpensslCrypto
{
public:
    OpensslDES();
    virtual ~OpensslDES();
private:
    const void* getCipher();
};

/*
    name       key     iv
    EDE-ECB    16B      N/A
    EDE-CBC    16B      16B
    EDE-CFB64  16B      16B
    EDE-OFB    16B      16B
*/
class COM_EXPORT Openssl2DES : public OpensslCrypto
{
public:
    Openssl2DES();
    virtual ~Openssl2DES();
private:
    const void* getCipher();
};

/*
    name        key     iv
    EDE-ECB    32B      N/A
    EDE-CBC    32B      32B
    EDE-CFB1   32B      32B
    EDE-CFB8   32B      32B
    EDE-CFB64  32B      32B
    EDE-OFB    32B      32B
*/
class COM_EXPORT Openssl3DES : public OpensslCrypto
{
public:
    Openssl3DES();
    virtual ~Openssl3DES();
private:
    const void* getCipher();
};

/*
    name   key      iv
    ECB    16B      N/A
    CBC    16B      16B
*/
class COM_EXPORT OpensslSM4 : public OpensslCrypto
{
public:
    OpensslSM4();
    virtual ~OpensslSM4();
private:
    const void* getCipher();
};

/*
    RSA秘钥生成：

    创建私钥
    openssl genrsa -out rsa_private_key.pem 1024  #1024是秘钥位数，为保证安全，私钥不能低于1024位
    如果需要给私钥加密码：
    openssl genrsa -des3 -passout pass:123456 -out rsa_private_key.pem 1024

    根据私钥创建公钥
    openssl rsa -in rsa_private_key.pem -pubout -out rsa_public_key.pem
    如果私钥有密码则需要输入密码或从命令行提供一个密码
    openssl rsa -in rsa_private_key.pem -passin pass:123456 -pubout -out rsa_public_key.pem
*/
class COM_EXPORT OpensslRSA
{
public:
    OpensslRSA();
    ~OpensslRSA();
    bool setPublicKey(const char* public_key_pem, const char* pwd = NULL);
    bool setPrivateKey(const char* private_key_pem, const char* pwd = NULL);
    bool setPublicKeyFromFile(const char* file_public_key, const char* pwd = NULL);
    bool setPrivateKeyFromFile(const char* file_private_key, const char* pwd = NULL);
    void setPaddingPKCS1();
    void setPaddingSSLV23();
    void setPaddingNO();
    void setPaddingPKCS1OAEP();
    void setPaddingX931();
    CPPBytes encryptWithPublicKey(uint8* data, int data_size);
    CPPBytes decryptWithPrivateKey(uint8* data, int data_size);
    CPPBytes encryptWithPrivateKey(uint8* data, int data_size);
    CPPBytes decryptWithPublicKey(uint8* data, int data_size);
    void cleanPublicKey();
    void cleanPrivateKey();
public:
    static bool GenerateKey(int key_bits, std::string& publc_key, std::string& private_key, const char* pwd = NULL);
private:
    std::string file_public_key;
    std::string file_private_key;
    void* key_pub;
    void* key_pri;
    int padding_mode;
};

class CertDistinguishedName
{
public:
    CertDistinguishedName();
    virtual ~CertDistinguishedName();
    std::string toString();
    static CertDistinguishedName FromString(const char* str);

public:
    std::string CN;
    std::string O;
    std::string OU;
    std::string L;
    std::string S;
    std::string C;
    std::string E;
    std::string G;
};

class COM_EXPORT OpensslCert
{
public:
    OpensslCert();
    virtual ~OpensslCert();

    bool loadFromFile(const char* file);
    bool loadFromMem(const char* data);
    bool loadFromMem(const void* data, int data_size);

    int ver;
    std::string sn;
    std::string signature_algorithm;
    std::string issuer;
    uint64 valid_begin;
    uint64 valid_end;
    std::string subject;
    bool expired;
    bool isCA;
    std::string public_key_algorithm;
    std::string public_key;
    std::map<std::string, std::string> ext;

    static bool CreateCert(Message& params);
public:
    std::string getIssuer(const char* tag = NULL);
    std::string getSubject(const char* tag = NULL);
};


#endif /* __COMEX_OPENSSL_H__ */

