#include <openssl/err.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/des.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/pkcs12.h>
#include <openssl/x509v3.h>

#include "comex_openssl.h"

static int openssl_password_cb(char* buf, int size, int rwflag, void* userdata)
{
    if(buf == NULL || userdata == NULL || size <= 0)
    {
        return -1;
    }
    //rwflag:read=0,write=1
    com_strncpy(buf, (char*)userdata, size);
    buf[size - 1] = '\0';
    return (int)strlen(buf);
}

OpensslHash::OpensslHash(const char* type)
{
    ctx = EVP_MD_CTX_create();
    if(ctx == NULL)
    {
        LOG_E("failed to create ctx");
        return;
    }

    OpenSSL_add_all_digests();
    digest = EVP_get_digestbyname(type);
    if(digest == NULL)
    {
        LOG_E("not support:%s", type);
        return;
    }

    if(EVP_DigestInit_ex((EVP_MD_CTX*)ctx, (const EVP_MD*)digest, NULL) != 1)
    {
        LOG_E("failed to int digest");
    }
}

OpensslHash::~OpensslHash()
{
    if(ctx != NULL)
    {
        EVP_MD_CTX_destroy((EVP_MD_CTX*)ctx);
        ctx = NULL;
    }
}

bool OpensslHash::append(const void* data, int data_size)
{
    if(ctx == NULL || digest == NULL || data == NULL || data_size <= 0)
    {
        return false;
    }
    if(EVP_DigestUpdate((EVP_MD_CTX*)ctx, data, data_size) != 1)
    {
        LOG_E("failed to update");
        return false;
    }
    return true;
}

CPPBytes OpensslHash::finish()
{
    if(ctx == NULL || digest == NULL)
    {
        return CPPBytes();
    }
    uint32 size_out = 0;
    uint8 buf[EVP_MAX_MD_SIZE];
    memset(buf, 0, sizeof(buf));
    if(EVP_DigestFinal_ex((EVP_MD_CTX*)ctx, buf, &size_out) != 1)
    {
        LOG_E("failed to finalize");
        return false;
    }
    CPPBytes hash = CPPBytes(buf, size_out);
    EVP_DigestInit_ex((EVP_MD_CTX*)ctx, (const EVP_MD*)digest, NULL);
    return hash;
}

CPPBytes OpensslHash::Digest(const char* type, const void* data, int data_size)
{
    uint32 size_out = 0;
    uint8 buf[EVP_MAX_MD_SIZE];
    memset(buf, 0, sizeof(buf));
    OpenSSL_add_all_digests();
    if(EVP_Digest((const uint8*)data, data_size, buf, &size_out, EVP_get_digestbyname(type), NULL) != 1)
    {
        return CPPBytes();
    }
    return CPPBytes(buf, size_out);
}

OpensslHMAC::OpensslHMAC(const char* type)
{
    ctx = HMAC_CTX_new();
    if(ctx == NULL)
    {
        LOG_E("failed to create ctx");
        return;
    }

    OpenSSL_add_all_digests();
    digest = EVP_get_digestbyname(type);
    if(digest == NULL)
    {
        LOG_E("not support:%s", type);
        return;
    }

    if(HMAC_Init_ex((HMAC_CTX*)ctx, key.empty() ? NULL : key.data(), (int)key.size(), (const EVP_MD*)digest, NULL) != 1)
    {
        LOG_E("failed to int digest");
    }
}

OpensslHMAC::~OpensslHMAC()
{
    if(ctx != NULL)
    {
        HMAC_CTX_free((HMAC_CTX*)ctx);
        ctx = NULL;
    }
}

bool OpensslHMAC::setKey(const char* key)
{
    return setKey((const uint8*)key, com_string_size(key));
}

bool OpensslHMAC::setKey(const uint8* key, int key_size)
{
    if(key == NULL || key_size <= 0)
    {
        return false;
    }
    this->key.clear();
    for(int i = 0; i < key_size; i++)
    {
        this->key.push_back(key[i]);
    }
    return true;
}

bool OpensslHMAC::append(const void* data, int data_size)
{
    if(ctx == NULL || digest == NULL || data == NULL || data_size <= 0)
    {
        return false;
    }
    if(HMAC_Update((HMAC_CTX*)ctx, (const uint8*)data, data_size) != 1)
    {
        LOG_E("failed to update");
        return false;
    }
    return true;
}

CPPBytes OpensslHMAC::finish()
{
    if(ctx == NULL || digest == NULL)
    {
        return CPPBytes();
    }
    unsigned int size_out = 0;
    uint8 buf[EVP_MAX_MD_SIZE];
    memset(buf, 0, sizeof(buf));
    if(HMAC_Final((HMAC_CTX*)ctx, buf, &size_out) != 1)
    {
        LOG_E("failed to finalize");
        return false;
    }
    CPPBytes hash = CPPBytes(buf, size_out);
    if(digest != NULL && digest != NULL)
    {
        HMAC_Init_ex((HMAC_CTX*)ctx, key.empty() ? NULL : key.data(), (int)key.size(), (const EVP_MD*)digest, NULL);
    }
    return hash;
}

CPPBytes OpensslHMAC::Digest(const char* type, const void* data, int data_size, const void* key, int key_size)
{
    uint32 size_out = 0;
    uint8 buf[EVP_MAX_MD_SIZE];
    memset(buf, 0, sizeof(buf));
    if(key_size == 0)
    {
        key_size = com_string_len((const char*)key);
    }
    OpenSSL_add_all_digests();
    HMAC(EVP_get_digestbyname(type), key, key_size, (const uint8*)data, data_size, buf, &size_out);
    return CPPBytes(buf, size_out);
}

OpensslCrypto::OpensslCrypto()
{
    ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init((EVP_CIPHER_CTX*)ctx);
}

OpensslCrypto::~OpensslCrypto()
{
    EVP_CIPHER_CTX_cleanup((EVP_CIPHER_CTX*)ctx);
    EVP_CIPHER_CTX_free((EVP_CIPHER_CTX*)ctx);
}

bool OpensslCrypto::setMode(const char* mode)
{
    if(mode == NULL)
    {
        return false;
    }
    engine_mode = mode;
    com_string_to_upper(engine_mode);
    return true;
}

bool OpensslCrypto::setIV(const char* iv)
{
    return setIV((uint8*)iv, com_string_len(iv));
}

bool OpensslCrypto::setIV(const std::string& iv)
{
    return setIV((uint8*)iv.data(), (int)iv.length());
}

bool OpensslCrypto::setIV(uint8* iv_data, int iv_size)
{
    if(iv_data == NULL || iv_size <= 0)
    {
        LOG_E("iv incorrent");
        return false;
    }
    iv.clear();
    iv.append(iv_data, iv_size);
    return true;
}

bool OpensslCrypto::setKey(const char* key)
{
    return setKey((uint8*)key, com_string_len(key));
}

bool OpensslCrypto::setKey(const std::string& key)
{
    return setKey((uint8*)key.data(), (int)key.length());
}

bool OpensslCrypto::setKey(uint8* key, int key_size)
{
    if(key == NULL || key_size <= 0)
    {
        LOG_E("key incorrent");
        return false;
    }
    this->key.clear();
    this->key.append(key, key_size);
    return true;
}

bool OpensslCrypto::setTag(const char* tag)
{
    return setTag((uint8*)tag, com_string_len(tag));
}

bool OpensslCrypto::setTag(const std::string& tag)
{
    return setTag((uint8*)tag.data(), (int)tag.length());
}

bool OpensslCrypto::setTag(uint8* tag, int tag_size)
{
    if(tag == NULL || tag_size <= 0)
    {
        LOG_E("tag incorrent");
        return false;
    }
    this->tag.clear();
    this->tag.append(tag, tag_size);
    return true;
}

bool OpensslCrypto::setTag(const CPPBytes& tag)
{
    this->tag = tag;
    return true;
}

CPPBytes OpensslCrypto::getTag()
{
    return tag;
}

CPPBytes OpensslCrypto::encrypt(const uint8* data, int data_size)
{
    if(encryptBegin() == false)
    {
        return CPPBytes();
    }

    CPPBytes bytes;
    if(encryptAppend(bytes, data, data_size) == false)
    {
        return CPPBytes();
    }

    if(encryptEnd(bytes) == false)
    {
        return CPPBytes();
    }
    return bytes;
}

CPPBytes OpensslCrypto::decrypt(const uint8* data, int data_size)
{
    if(decryptBegin() == false)
    {
        return CPPBytes();
    }

    CPPBytes bytes;
    if(decryptAppend(bytes, data, data_size) == false)
    {
        return CPPBytes();
    }

    if(decryptEnd(bytes) == false)
    {
        return CPPBytes();
    }
    return bytes;
}

bool OpensslCrypto::encryptFile(const char* file_src, const char* file_dst, bool attach_tag)
{
    if(file_src == NULL || file_dst == NULL)
    {
        LOG_E("arg incorrect,file_src=%p,file_dst=%p", file_src, file_dst);
        return false;
    }

    if(encryptBegin() == false)
    {
        LOG_E("encryptBegin failed");
        return false;
    }

    FILE* fin = com_file_open(PATH_TO_LOCAL(file_src).c_str(), "rb");
    if(fin == NULL)
    {
        LOG_E("failed to open %s", file_src);
        return false;
    }

    FILE* fout = com_file_open(PATH_TO_LOCAL(file_dst).c_str(), "wb+");
    if(fout == NULL)
    {
        com_file_close(fin);
        LOG_E("failed to open %s", file_dst);
        return false;
    }

    bool ret = true;
    int len = 0;
    CPPBytes bytes;
    while((len = (int)com_file_read(fin, buf, sizeof(buf))) > 0)
    {
        bytes.clear();
        if(encryptAppend(bytes, buf, len) == false)
        {
            LOG_E("encryptAppend failed");
            ret = false;
            break;
        }
        if(com_file_write(fout, bytes.getData(), bytes.getDataSize()) != bytes.getDataSize())
        {
            LOG_E("file write failed");
            ret = false;
            break;
        }
    }
    bytes.clear();
    if(ret == false
            || encryptEnd(bytes) == false
            || com_file_write(fout, bytes.getData(), bytes.getDataSize()) != bytes.getDataSize())
    {
        LOG_E("encrypt end failed");
        ret = false;
    }
    if(attach_tag)
    {
        if(com_file_write(fout, tag.getData(), tag.getDataSize()) != tag.getDataSize())
        {
            LOG_E("failed to attach the tag");
            ret = false;
        }
    }
    com_file_close(fin);
    com_file_flush(fout);
    com_file_close(fout);
    return ret;
}

bool OpensslCrypto::decryptFile(const char* file_src, const char* file_dst)
{
    if(file_src == NULL || file_dst == NULL)
    {
        LOG_E("arg incorrect,file_src=%p,file_dst=%p", file_src, file_dst);
        return false;
    }

    if(decryptBegin() == false)
    {
        LOG_E("decryptBegin failed");
        return false;
    }

    FILE* fin = com_file_open(PATH_TO_LOCAL(file_src).c_str(), "rb");
    if(fin == NULL)
    {
        LOG_E("failed to open %s", file_src);
        return false;
    }

    FILE* fout = com_file_open(PATH_TO_LOCAL(file_dst).c_str(), "wb+");
    if(fout == NULL)
    {
        com_file_close(fin);
        LOG_E("failed to open %s", file_dst);
        return false;
    }

    bool ret = true;
    int len = 0;
    CPPBytes bytes;
    while((len = com_file_read(fin, buf, sizeof(buf))) > 0)
    {
        bytes.clear();
        if(decryptAppend(bytes, buf, len) == false)
        {
            LOG_E("decryptAppend failed");
            ret = false;
            break;
        }
        if(com_file_write(fout, bytes.getData(), bytes.getDataSize()) != bytes.getDataSize())
        {
            LOG_E("failed to write file");
            ret = false;
            break;
        }
    }
    bytes.clear();
    if(ret == false
            || decryptEnd(bytes) == false
            || com_file_write(fout, bytes.getData(), bytes.getDataSize()) != bytes.getDataSize())
    {
        LOG_E("decryptEnd failed:%s", file_src);
        ret = false;
    }
    com_file_close(fin);
    com_file_flush(fout);
    com_file_close(fout);
    return ret;
}

bool OpensslCrypto::encryptBegin()
{
    if(ctx == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    // 指定加密算法及key和iv
    EVP_CIPHER* cipher = (EVP_CIPHER*)getCipher();
    if(cipher == NULL)
    {
        LOG_E("failed to get cipher");
        return false;
    }

    if(EVP_EncryptInit_ex((EVP_CIPHER_CTX*)ctx, cipher, NULL, NULL, NULL) != 1)
    {
        LOG_E("failed to int cipher");
        return false;
    }

    if(engine_mode == "CCM")
    {
        if(EVP_CIPHER_CTX_ctrl((EVP_CIPHER_CTX*)ctx, EVP_CTRL_CCM_SET_TAG, OPENSSL_AES_TAG_SIZE, NULL) != 1)
        {
            LOG_E("failed to set tag,mode=%s", engine_mode.c_str());
            return false;
        }
    }
    else if(engine_mode == "GCM")
    {
#if 0
        if(EVP_CIPHER_CTX_ctrl((EVP_CIPHER_CTX*)ctx, EVP_CTRL_GCM_SET_TAG, OPENSSL_AES_TAG_SIZE, NULL) != 1)
        {
            LOG_E("failed to set tag,mode=%s", engine_mode.c_str());
            return false;
        }
#endif
    }

    if(EVP_EncryptInit_ex((EVP_CIPHER_CTX*)ctx, NULL, NULL, key.getData(), iv.getDataSize() > 0 ? iv.getData() : NULL) != 1)
    {
        LOG_E("failed to int key & iv");
        return false;
    }
    return true;
}

bool OpensslCrypto::encryptAppend(CPPBytes& result, const uint8* data, int data_size)
{
    if(ctx == NULL || data == NULL || data_size <= 0)
    {
        LOG_E("arg incorrect,ctx=%p,data=%p,data_size=%d", ctx, data, data_size);
        return false;
    }
    int size_out = 0;
    std::vector<uint8> buf;
    buf.reserve(data_size);
    if(EVP_EncryptUpdate((EVP_CIPHER_CTX*)ctx, buf.data(), &size_out, data, data_size) != 1)
    {
        LOG_E("failed to update");
        return false;
    }
    result.append(buf.data(), size_out);
    return true;
}

bool OpensslCrypto::encryptEnd(CPPBytes& result)
{
    if(ctx == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    // 结束加密操作
    int size_out = 0;
    uint8 buf[AES_BLOCK_SIZE];
    memset(buf, 0, sizeof(buf));
    if(EVP_EncryptFinal_ex((EVP_CIPHER_CTX*)ctx, buf, &size_out) != 1)
    {
        LOG_W("failed to EVP_EncryptFinal_ex");
        //return false;
    }
    if(size_out > 0)
    {
        result.append(buf, size_out);
    }

    if(engine_mode == "CCM" || engine_mode == "GCM")
    {
        uint8 buf_tag[OPENSSL_AES_TAG_SIZE];
        memset(buf_tag, 0, sizeof(buf_tag));
        if(EVP_CIPHER_CTX_ctrl((EVP_CIPHER_CTX*)ctx, engine_mode == "CCM" ? EVP_CTRL_CCM_GET_TAG : EVP_CTRL_GCM_GET_TAG, sizeof(buf_tag), buf_tag) != 1)
        {
            LOG_E("failed to get tag");
            return false;
        }
        tag = CPPBytes(buf_tag, sizeof(buf_tag));
    }

    return true;
}

bool OpensslCrypto::decryptBegin()
{
    if(ctx == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    EVP_CIPHER* cipher = (EVP_CIPHER*)getCipher();
    if(cipher == NULL)
    {
        LOG_E("failed to get cipher");
        return false;
    }
    if(EVP_DecryptInit_ex((EVP_CIPHER_CTX*)ctx, cipher, NULL, NULL, NULL) != 1)
    {
        LOG_E("failed to int cipher");
        return false;
    }
    if(engine_mode == "CCM" || engine_mode == "GCM")
    {
        if(EVP_CIPHER_CTX_ctrl((EVP_CIPHER_CTX*)ctx, EVP_CTRL_CCM_SET_TAG, tag.getDataSize(), tag.getData()) != 1)
        {
            LOG_E("failed to set tag");
            return false;
        }
    }

    // 指定解密算法及key和iv
    if(EVP_DecryptInit_ex((EVP_CIPHER_CTX*)ctx, NULL, NULL, key.getData(), iv.getDataSize() > 0 ? iv.getData() : NULL) != 1)
    {
        LOG_E("failed to int key & iv");
        return false;
    }

    return true;
}

bool OpensslCrypto::decryptAppend(CPPBytes& result, const uint8* data, int data_size)
{
    if(ctx == NULL || data == NULL || data_size <= 0)
    {
        LOG_E("arg incorrect,ctx=%p,data=%p,data_size=%d", ctx, data, data_size);
        return false;
    }
    int size_out = 0;
    std::vector<uint8> buf;
    buf.reserve(data_size);
    if(EVP_DecryptUpdate((EVP_CIPHER_CTX*)ctx, buf.data(), &size_out, data, data_size) != 1)
    {
        LOG_E("failed to update");
        return false;
    }
    result.append(buf.data(), size_out);
    return true;
}

bool OpensslCrypto::decryptEnd(CPPBytes& result)
{
    if(ctx == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    int size_out = 0;
    std::vector<uint8> buf;
    buf.reserve(AES_BLOCK_SIZE);
    if(EVP_DecryptFinal_ex((EVP_CIPHER_CTX*)ctx, buf.data(), &size_out) != 1)
    {
        LOG_W("failed,size_out=%d", size_out);
        return false;
    }
    if(size_out > 0)
    {
        result.append(buf.data(), size_out);
    }
    return true;
}

OpensslAES::OpensslAES()
{
}

OpensslAES::~OpensslAES()
{
}

const void* OpensslAES::getCipher()
{
    if(engine_mode == "ECB")
    {
        if(key.getDataSize() == 16)
        {
            return EVP_aes_128_ecb();
        }
        else if(key.getDataSize() == 24)
        {
            return EVP_aes_192_ecb();
        }
        else if(key.getDataSize() == 32)
        {
            return EVP_aes_256_ecb();
        }
        else
        {
            LOG_E("%s key size incorrect:%d,must be 16B,24B or 32B", engine_mode.c_str(), key.getDataSize());
        }
    }
    else if(engine_mode == "CBC")
    {
        if(key.getDataSize() == 16)
        {
            return EVP_aes_128_cbc();
        }
        else if(key.getDataSize() == 24)
        {
            return EVP_aes_192_cbc();
        }
        else if(key.getDataSize() == 32)
        {
            return EVP_aes_256_cbc();
        }
        else
        {
            LOG_E("%s key size incorrect:%d,must be 16B,24B or 32B", engine_mode.c_str(), key.getDataSize());
        }
    }
    else if(engine_mode == "CFB1")
    {
        if(key.getDataSize() == 16)
        {
            return EVP_aes_128_cfb1();
        }
        else if(key.getDataSize() == 24)
        {
            return EVP_aes_192_cfb1();
        }
        else if(key.getDataSize() == 24)
        {
            return EVP_aes_256_cfb1();
        }
        else
        {
            LOG_E("%s key size incorrect:%d,must be 16B,24B or 32B", engine_mode.c_str(), key.getDataSize());
        }
    }
    else if(engine_mode == "CFB8")
    {
        if(key.getDataSize() == 16)
        {
            return EVP_aes_128_cfb8();
        }
        else if(key.getDataSize() == 24)
        {
            return EVP_aes_192_cfb8();
        }
        else if(key.getDataSize() == 32)
        {
            return EVP_aes_256_cfb8();
        }
        else
        {
            LOG_E("%s key size incorrect:%d,must be 16B,24B or 32B", engine_mode.c_str(), key.getDataSize());
        }
    }
    else if(engine_mode == "CFB128")
    {
        if(key.getDataSize() == 16)
        {
            return EVP_aes_128_cfb128();
        }
        else if(key.getDataSize() == 24)
        {
            return EVP_aes_192_cfb128();
        }
        else if(key.getDataSize() == 32)
        {
            return EVP_aes_256_cfb128();
        }
        else
        {
            LOG_E("%s key size incorrect:%d,must be 16B,24B or 32B", engine_mode.c_str(), key.getDataSize());
        }
    }
    else if(engine_mode == "OFB")
    {
        if(key.getDataSize() == 16)
        {
            return EVP_aes_128_ofb();
        }
        else if(key.getDataSize() == 24)
        {
            return EVP_aes_192_ofb();
        }
        else if(key.getDataSize() == 32)
        {
            return EVP_aes_256_ofb();
        }
        else
        {
            LOG_E("%s key size incorrect:%d,must be 16B,24B or 32B", engine_mode.c_str(), key.getDataSize());
        }
    }
    else if(engine_mode == "CTR")
    {
        if(key.getDataSize() == 16)
        {
            return EVP_aes_128_ctr();
        }
        else if(key.getDataSize() == 24)
        {
            return EVP_aes_192_ctr();
        }
        else if(key.getDataSize() == 32)
        {
            return EVP_aes_256_ctr();
        }
        else
        {
            LOG_E("%s key size incorrect:%d,must be 16B,24B or 32B", engine_mode.c_str(), key.getDataSize());
        }
    }
    else if(engine_mode == "CCM")
    {
        if(key.getDataSize() == 16)
        {
            return EVP_aes_128_ccm();
        }
        else if(key.getDataSize() == 24)
        {
            return EVP_aes_192_ccm();
        }
        else if(key.getDataSize() == 32)
        {
            return EVP_aes_256_ccm();
        }
        else
        {
            LOG_E("%s key size incorrect:%d,must be 16B,24B or 32B", engine_mode.c_str(), key.getDataSize());
        }
    }
    else if(engine_mode == "GCM")
    {
        if(key.getDataSize() == 16)
        {
            return EVP_aes_128_gcm();
        }
        else if(key.getDataSize() == 24)
        {
            return EVP_aes_192_gcm();
        }
        else if(key.getDataSize() == 32)
        {
            return EVP_aes_256_gcm();
        }
        else
        {
            LOG_E("%s key size incorrect:%d,must be 16B,24B or 32B", engine_mode.c_str(), key.getDataSize());
        }
    }
    else if(engine_mode == "XTS")
    {
        if(key.getDataSize() == 32)
        {
            return EVP_aes_128_xts();
        }
        else if(key.getDataSize() == 64)
        {
            return EVP_aes_256_xts();
        }
        else
        {
            LOG_E("%s key size incorrect:%d,must be 32B,64B", engine_mode.c_str(), key.getDataSize());
        }
    }
    else
    {
        LOG_E("not support:%s", engine_mode.c_str());
    }
    return NULL;
}

OpensslDES::OpensslDES()
{
}

OpensslDES::~OpensslDES()
{
}

const void* OpensslDES::getCipher()
{
    if(key.getDataSize() != 8)
    {
        LOG_E("%s key size incorrect:%d,must be 8B", engine_mode.c_str(), key.getDataSize());
        return NULL;
    }
    if(engine_mode == "ECB")
    {
        return EVP_des_ecb();
    }
    else if(engine_mode == "CBC")
    {
        return EVP_des_cbc();
    }
    else if(engine_mode == "CFB1")
    {
        return EVP_des_cfb1();
    }
    else if(engine_mode == "CFB8")
    {
        return EVP_des_cfb8();
    }
    else if(engine_mode == "CFB64")
    {
        return EVP_des_cfb64();
    }
    else
    {
        LOG_E("not support:%s", engine_mode.c_str());
    }
    return NULL;
}

Openssl2DES::Openssl2DES()
{
}

Openssl2DES::~Openssl2DES()
{
}

const void* Openssl2DES::getCipher()
{
    if(key.getDataSize() != 16)
    {
        LOG_E("%s key size incorrect:%d,must be 16B", engine_mode.c_str(), key.getDataSize());
        return NULL;
    }
    if(engine_mode == "EDE-ECB")
    {
        return EVP_des_ede_ecb();
    }
    else if(engine_mode == "EDE-CBC")
    {
        return EVP_des_ede_cbc();
    }
    else if(engine_mode == "EDE-CFB64")
    {
        return EVP_des_ede_cfb64();
    }
    else if(engine_mode == "EDE-OFB")
    {
        return EVP_des_ede_ofb();
    }
    else
    {
        LOG_E("not support:%s", engine_mode.c_str());
    }
    return NULL;
}

Openssl3DES::Openssl3DES()
{
}

Openssl3DES::~Openssl3DES()
{
}

const void* Openssl3DES::getCipher()
{
    if(key.getDataSize() != 24)
    {
        LOG_E("%s key size incorrect:%d,must be 24B", engine_mode.c_str(), key.getDataSize());
        return NULL;
    }
    if(engine_mode == "EDE-ECB")
    {
        return EVP_des_ede3_ecb();
    }
    else if(engine_mode == "EDE-CBC")
    {
        return EVP_des_ede3_cbc();
    }
    else if(engine_mode == "EDE-CFB1")
    {
        return EVP_des_ede3_cfb1();
    }
    else if(engine_mode == "EDE-CFB8")
    {
        return EVP_des_ede3_cfb8();
    }
    else if(engine_mode == "EDE-CFB64")
    {
        return EVP_des_ede3_cfb64();
    }
    else if(engine_mode == "EDE-OFB")
    {
        return EVP_des_ede3_ofb();
    }
    else
    {
        LOG_E("not support:%s", engine_mode.c_str());
    }
    return NULL;
}

OpensslRSA::OpensslRSA()
{
    padding_mode = 1;
    rsa_pub = NULL;
    rsa_priv = NULL;
}

OpensslRSA::~OpensslRSA()
{
    cleanPrivateKey();
    cleanPublicKey();
}

bool OpensslRSA::setPublicKey(const char* public_key_pem, const char* pwd)
{
    if(public_key_pem == NULL)
    {
        return false;
    }
    BIO* b = BIO_new(BIO_s_mem());
    if(b == NULL)
    {
        LOG_E("failed to new bio");
        return false;
    }
    int ret = BIO_write(b, public_key_pem, com_string_len(public_key_pem));
    if(ret <= 0)
    {
        BIO_free(b);
        LOG_E("failed write bio");
        return false;
    }
    cleanPublicKey();
    rsa_pub = PEM_read_bio_RSAPublicKey(b, NULL, pwd == NULL ? NULL : openssl_password_cb, (void*)pwd);
    if(rsa_pub == NULL)
    {
        BIO_free(b);
        LOG_E("failed load key from bio");
        return false;
    }
    BIO_free(b);
    return true;
}

bool OpensslRSA::setPrivateKey(const char* private_key_pem, const char* pwd)
{
    if(private_key_pem == NULL)
    {
        return false;
    }
    BIO* b = BIO_new(BIO_s_mem());
    if(b == NULL)
    {
        LOG_E("failed to new bio");
        return false;
    }
    int ret = BIO_write(b, private_key_pem, com_string_len(private_key_pem));
    if(ret <= 0)
    {
        BIO_free(b);
        LOG_E("failed write bio");
        return false;
    }
    cleanPrivateKey();
    rsa_priv = PEM_read_bio_RSAPrivateKey(b, NULL, pwd == NULL ? NULL : openssl_password_cb, (void*)pwd);
    if(rsa_priv == NULL)
    {
        BIO_free(b);
        LOG_E("failed load key from bio");
        return false;
    }
    BIO_free(b);
    return true;
}

bool OpensslRSA::setPublicKeyFromFile(const char* file_public_key, const char* pwd)
{
    if(file_public_key == NULL)
    {
        return false;
    }
    cleanPublicKey();
    FILE* file = com_file_open(PATH_TO_LOCAL(file_public_key).c_str(), "r");
    if(file == NULL)
    {
        return false;
    }
    rsa_pub = PEM_read_RSA_PUBKEY(file, NULL, openssl_password_cb, (void*)pwd);
    com_file_close(file);
    if(rsa_pub == NULL)
    {
        LOG_E("load public key failed");
        return false;
    }
    return true;
}

bool OpensslRSA::setPrivateKeyFromFile(const char* file_private_key, const char* pwd)
{
    if(file_private_key == NULL)
    {
        return false;
    }
    cleanPrivateKey();
    FILE* file = com_file_open(PATH_TO_LOCAL(file_private_key).c_str(), "r");
    if(file == NULL)
    {
        return false;
    }
    rsa_priv = PEM_read_RSAPrivateKey(file, NULL, openssl_password_cb, (void*)pwd);
    com_file_close(file);
    if(rsa_priv == NULL)
    {
        LOG_E("load priv key failed");
        return false;
    }
    return true;
}

void OpensslRSA::setPaddingPKCS1()
{
    padding_mode = 1;
}

void OpensslRSA::setPaddingSSLV23()
{
    padding_mode = 2;
}

void OpensslRSA::setPaddingNO()
{
    padding_mode = 3;
}

void OpensslRSA::setPaddingPKCS1OAEP()
{
    padding_mode = 4;
}

void OpensslRSA::setPaddingX931()
{
    padding_mode = 5;
}

CPPBytes OpensslRSA::encryptWithPublicKey(uint8* data, int data_size)
{
    CPPBytes bytes;
    if(data == NULL || data_size <= 0 || rsa_pub == NULL)
    {
        LOG_E("arg incorrect");
        return CPPBytes();
    }
    int rsa_size = RSA_size((RSA*)rsa_pub);
    if(rsa_size <= 0)
    {
        return CPPBytes();
    }
    std::vector<uint8> buf;
    buf.reserve(rsa_size);
    int ret = RSA_public_encrypt(data_size, data, buf.data(), (RSA*)rsa_pub, padding_mode);
    if(ret <= 0)
    {
        LOG_E("rsa public encrypt failed,ret=%d,err=0x%lX", ret, ERR_get_error());
        return CPPBytes();
    }
    return CPPBytes(buf.data(), rsa_size);;
}

CPPBytes OpensslRSA::decryptWithPrivateKey(uint8* data, int data_size)
{
    CPPBytes bytes;
    if(data == NULL || data_size <= 0 || rsa_priv == NULL)
    {
        return bytes;
    }
    int rsa_size = RSA_size((RSA*)rsa_priv);
    if(rsa_size <= 0)
    {
        return CPPBytes();
    }
    std::vector<uint8> buf;
    buf.reserve(rsa_size);
    int ret = RSA_private_decrypt(data_size, data, buf.data(), (RSA*)rsa_priv, padding_mode);
    if(ret <= 0)
    {
        LOG_E("rsa private decrypt failed,ret=%d,err=0x%lX", ret, ERR_get_error());
        return CPPBytes();
    }
    return CPPBytes(buf.data(), rsa_size);
}

CPPBytes OpensslRSA::encryptWithPrivaeKey(uint8* data, int data_size)
{
    CPPBytes bytes;
    if(data == NULL || data_size <= 0 || rsa_priv == NULL)
    {
        return bytes;
    }
    int rsa_size = RSA_size((RSA*)rsa_priv);
    if(rsa_size <= 0)
    {
        return CPPBytes();
    }
    std::vector<uint8> buf;
    buf.reserve(rsa_size);
    int ret = RSA_private_encrypt(data_size, data, buf.data(), (RSA*)rsa_priv, padding_mode);
    if(ret <= 0)
    {
        LOG_E("rsa private encrypt failed,ret=%d,err=0x%lX", ret, ERR_get_error());
        return CPPBytes();
    }
    return CPPBytes(buf.data(), rsa_size);
}

CPPBytes OpensslRSA::decryptWithPublicKey(uint8* data, int data_size)
{
    CPPBytes bytes;
    if(data == NULL || data_size <= 0 || rsa_pub == NULL)
    {
        return bytes;
    }
    int rsa_size = RSA_size((RSA*)rsa_pub);
    if(rsa_size <= 0)
    {
        return CPPBytes();
    }
    std::vector<uint8> buf;
    buf.reserve(rsa_size);
    int ret = RSA_public_decrypt(data_size, data, buf.data(), (RSA*)rsa_pub, padding_mode);
    if(ret <= 0)
    {
        LOG_E("rsa public decrypt failed,ret=%d,err=0x%lX", ret, ERR_get_error());
        return CPPBytes();
    }
    return CPPBytes(buf.data(), rsa_size);
}

void OpensslRSA::cleanPublicKey()
{
    if(rsa_pub != NULL)
    {
        RSA_free((RSA*)rsa_pub);
        rsa_pub = NULL;
    }
}

void OpensslRSA::cleanPrivateKey()
{
    if(rsa_priv != NULL)
    {
        RSA_free((RSA*)rsa_priv);
        rsa_priv = NULL;
    }
}

bool OpensslRSA::GenerateKey(int key_bits, std::string& public_key, std::string& private_key, const char* pwd)
{
    RSA* rsa = RSA_new();
    if(rsa == NULL)
    {
        return false;
    }
    BIGNUM* bn = BN_new();
    if(bn == NULL)
    {
        RSA_free(rsa);
        return false;
    }
    int ret = BN_set_word(bn, RSA_F4);
    if(ret != 1)
    {
        BN_free(bn);
        RSA_free(rsa);
        return false;
    }
    ret = RSA_generate_key_ex(rsa, key_bits, bn, NULL);
    if(ret != 1)
    {
        BN_free(bn);
        RSA_free(rsa);
        return false;
    }

    BIO* b1 = BIO_new(BIO_s_mem());
    if(b1 == NULL)
    {
        BN_free(bn);
        RSA_free(rsa);
        return false;
    }
    BIO* b2 = BIO_new(BIO_s_mem());
    if(b1 == NULL)
    {
        BIO_free_all(b1);
        BN_free(bn);
        RSA_free(rsa);
        return false;
    }
    PEM_write_bio_RSAPublicKey(b1, rsa);
    PEM_write_bio_RSAPrivateKey(b2, rsa, NULL, NULL, 0, pwd == NULL ? NULL : openssl_password_cb, (void*)pwd);

    char* pub_data = NULL;
    char* pri_data = NULL;
    int pub_size = BIO_get_mem_data(b1, &pub_data);
    int pri_size = BIO_get_mem_data(b2, &pri_data);
    if(pub_size <= 0 || pub_data == NULL || pri_size <= 0 || pri_data == NULL)
    {
        BIO_free_all(b1);
        BIO_free_all(b2);
        BN_free(bn);
        RSA_free(rsa);
        return false;
    }

    public_key = std::string(pub_data, pub_size);
    private_key = std::string(pri_data, pri_size);

    BIO_free_all(b1);
    BIO_free_all(b2);
    BN_free(bn);
    RSA_free(rsa);
    return true;
}

CertDistinguishedName::CertDistinguishedName()
{
}

CertDistinguishedName::~CertDistinguishedName()
{
}

std::string CertDistinguishedName::toString()
{
    return std::string();
}

CertDistinguishedName CertDistinguishedName::FromString(const char* str)
{
    if(com_string_len(str) <= 0)
    {
        return CertDistinguishedName();
    }
    return CertDistinguishedName();
}


OpensslCert::OpensslCert()
{
}

OpensslCert::~OpensslCert()
{
}

bool OpensslCert::loadFromFile(const char* file)
{
    CPPBytes bytes = com_file_readall(file);
    return loadFromMem(bytes.getData(), bytes.getDataSize());
}

bool OpensslCert::loadFromMem(const char* data)
{
    return loadFromMem(data, com_string_len(data));
}

bool OpensslCert::loadFromMem(const void* data, int data_size)
{
    if(data == NULL || data_size <= 0)
    {
        LOG_E("arg incorrect");
        return false;
    }

    BIO* bio = BIO_new(BIO_s_mem());
    if(bio == NULL)
    {
        LOG_E("failed");
        return false;
    }
    size_t size_written = 0;
    if(BIO_write_ex(bio, data, data_size, &size_written) != 1)
    {
        BIO_free(bio);
        LOG_E("failed write bio");
        return false;
    }
    X509* x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL);
    if(x509 == NULL)
    {
        BIO_free(bio);
        LOG_E("failed load pem cert");
        return false;
    }

    ver = (int)X509_get_version(x509);
    const ASN1_INTEGER* ais = X509_get_serialNumber(x509);
    if(ais != NULL)
    {
        sn = com_bytes_to_hexstring(ais->data, ais->length);
    }

    std::vector<char> buf;
    buf.reserve(1024 * 1024);
    issuer = X509_NAME_oneline(X509_get_issuer_name(x509), buf.data(), 1024 * 1024);
    subject = X509_NAME_oneline(X509_get_subject_name(x509), buf.data(), 1024 * 1024);

    const X509_ALGOR* sig_algor =  X509_get0_tbs_sigalg(x509);
    if(sig_algor != NULL)
    {
        const char* val = OBJ_nid2ln(OBJ_obj2nid(sig_algor->algorithm));
        if(val != NULL)
        {
            signature_algorithm = val;
        }
    }

    ASN1_TIME* not_before = X509_get_notBefore(x509);
    ASN1_TIME* not_after = X509_get_notAfter(x509);

    if(not_before != NULL && not_after != NULL)
    {
        struct tm tm;
        memset(&tm, 0, sizeof(struct tm));
        ASN1_TIME_to_tm(not_before, &tm);
        valid_begin = com_time_from_tm(&tm);
        memset(&tm, 0, sizeof(struct tm));
        ASN1_TIME_to_tm(not_after, &tm);
        valid_end = com_time_from_tm(&tm);
    }

    X509_free(x509);
    BIO_free(bio);
    return true;
}

std::string OpensslCert::getIssuer(const char* tag)
{
    if(tag == NULL)
    {
        return issuer;
    }
    std::vector<std::string> vals = com_string_split(issuer.c_str(), "/");
    for(size_t i = 0; i < vals.size(); i++)
    {
        std::size_t pos = vals[i].find_first_of("/");
        if(pos == std::string::npos)
        {
            continue;
        }
        std::string key = vals[i].substr(0, pos);
        std::string value = vals[i].substr(pos + 1);
        if(key == tag)
        {
            return value;
        }
    }
    return std::string();
}

std::string OpensslCert::getSubject(const char* tag)
{
    if(tag == NULL)
    {
        return subject;
    }
    std::vector<std::string> vals = com_string_split(subject.c_str(), "/");
    for(size_t i = 0; i < vals.size(); i++)
    {
        std::size_t pos = vals[i].find_first_of("/");
        if(pos == std::string::npos)
        {
            continue;
        }
        std::string key = vals[i].substr(0, pos);
        std::string value = vals[i].substr(pos + 1);
        if(key == tag)
        {
            return value;
        }
    }
    return std::string();
}

bool OpensslCert::CreateCert(Message& params)
{
    EVP_PKEY* key = NULL;
    RSA* rsa = NULL;
    BIGNUM* bn = NULL;
    X509* x509 = NULL;
    PKCS12* p12 = NULL;
    bool result = false;

    OpenSSL_add_all_digests();

    do
    {
        key = EVP_PKEY_new();
        if(key == NULL)
        {
            LOG_E("failed");
            break;
        }
        rsa = RSA_new();
        if(rsa == NULL)
        {
            LOG_E("failed");
            break;
        }
        bn = BN_new();
        if(bn == NULL)
        {
            LOG_E("failed");
            break;
        }
        BN_set_word(bn, RSA_F4);
        RSA_generate_key_ex(rsa, params.getInt32("key_bits", 2048), bn, NULL);
        EVP_PKEY_assign_RSA(key, rsa);

        if(params.isKeyExist("file_private_key"))
        {
            std::string pwd = params.getString("password_private_key");
            std::string file =  params.getString("file_private_key");
            FILE* f = com_file_open(file.c_str(), "w+");
            if(pwd.empty())
            {
                PEM_write_PrivateKey(f, key, NULL, NULL, 0, NULL, NULL);
            }
            else
            {
                PEM_write_PrivateKey(f, key, EVP_des_ede3_cbc(), (unsigned char*)pwd.c_str(), pwd.length(), NULL, NULL);
            }
            com_file_flush(f);
            com_file_close(f);
        }

        x509 = X509_new();
        if(x509 == NULL)
        {
            LOG_E("failed");
            break;
        }

        //set ver
        X509_set_version(x509, params.getInt8("ver", 2));

        //set sn
        ASN1_INTEGER_set(X509_get_serialNumber(x509), params.getUInt64("sn", com_time_rtc_ms()));

        //set valid time
        X509_gmtime_adj(X509_get_notBefore(x509), 0);
        X509_gmtime_adj(X509_get_notAfter(x509), params.getInt64("valid_year", 1) * 365 * 24 * 3600);

        //set key
        X509_set_pubkey(x509, key);

        //set subject
        std::string subject_str = params.getString("subject");
        if(subject_str.empty() == false)
        {
            std::vector<std::string> vals = com_string_split(subject_str.c_str(), ",");
            for(size_t i = 0; i < vals.size(); i++)
            {
                std::vector<std::string> kv = com_string_split(vals[i].c_str(), "=");
                if(kv.size() != 2)
                {
                    continue;
                }
                X509_NAME_add_entry_by_txt(X509_get_subject_name(x509), kv[0].c_str(), MBSTRING_ASC, (const unsigned char*)kv[1].c_str(), -1, -1, 0);
            }
        }

        //set issuer
        std::string issuer_str = params.getString("issuer");
        if(issuer_str.empty() == false)
        {
            std::vector<std::string> vals = com_string_split(issuer_str.c_str(), ",");
            for(size_t i = 0; i < vals.size(); i++)
            {
                std::vector<std::string> kv = com_string_split(vals[i].c_str(), "=");
                if(kv.size() != 2)
                {
                    continue;
                }
                X509_NAME_add_entry_by_txt(X509_get_issuer_name(x509), kv[0].c_str(), MBSTRING_ASC, (const unsigned char*)kv[1].c_str(), -1, -1, 0);
            }
        }
        else
        {
            X509_set_issuer_name(x509, X509_get_subject_name(x509));
        }

        if(params.isKeyExist("alt_name"))
        {
            std::string alt_name;
            std::vector<std::string> vals = com_string_split(params.getString("alt_name").c_str(), ",");
            for(size_t i = 0; i < vals.size(); i++)
            {
                if(com_string_is_ip(vals[i].c_str()))
                {
                    alt_name += "IP:" + vals[i] + ",";
                }
                else
                {
                    alt_name += "DNS:" + vals[i] + ",";
                }
            }
            if(alt_name.back() == ',')
            {
                alt_name.pop_back();
            }
            X509_EXTENSION* ext_alt_name = X509V3_EXT_conf_nid(NULL, NULL, NID_subject_alt_name, alt_name.c_str());
            if(ext_alt_name != NULL)
            {
                X509_add_ext(x509, ext_alt_name, -1);
                X509_EXTENSION_free(ext_alt_name);
            }
        }

        //sign
        if(X509_sign(x509, key, EVP_get_digestbyname(params.getString("sign_algorithm", "SHA256").c_str())) <= 0)
        {
            LOG_E("sign failed");
            break;
        }

        if(params.isKeyExist("file_crt"))
        {
            std::string file = params.getString("file_crt");
            FILE* f = com_file_open(file.c_str(), "w+");
            if(f == NULL)
            {
                LOG_E("failed:%s", file.c_str());
                break;
            }
            if(PEM_write_X509(f, x509) != 1)
            {
                com_file_close(f);
                LOG_E("failed to save cert to file:%s", file.c_str());
                break;
            }
            com_file_flush(f);
            com_file_close(f);
        }

        std::string pwd = params.getString("password_private_key");
        p12 = PKCS12_create(pwd.c_str(), NULL, key, x509, NULL, 0, 0, 0, 0, 0);
        if(p12 == NULL)
        {
            LOG_E("failed");
            break;
        }

        if(params.isKeyExist("file_p12"))
        {
            std::string file = params.getString("file_p12");
            FILE* f = com_file_open(file.c_str(), "w+");
            if(f == NULL)
            {
                LOG_E("failed:%s", file.c_str());
                break;
            }
            if(i2d_PKCS12_fp(f, p12) != 1)
            {
                com_file_close(f);
                LOG_E("failed to save cert to file:%s", file.c_str());
                break;
            }
            com_file_flush(f);
            com_file_close(f);
        }

        result = true;
    }
    while(0);
    if(x509 != NULL)
    {
        X509_free(x509);
        x509 = NULL;
    }
    if(key != NULL)
    {
        EVP_PKEY_free(key);
        key = NULL;
    }
    if(p12 != NULL)
    {
        PKCS12_free(p12);
        p12 = NULL;
    }
    if(bn != NULL)
    {
        BN_free(bn);
        bn = NULL;
    }
    return result;
}

