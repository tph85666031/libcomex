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
#include <openssl/evp.h>
#include <openssl/params.h>
#include <openssl/decoder.h>
#include <openssl/provider.h>

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
    OSSL_PROVIDER_try_load(NULL, "legacy", 1);
    //OSSL_PROVIDER_try_load(NULL, "default", 1);
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

ComBytes OpensslHash::finish()
{
    if(ctx == NULL || digest == NULL)
    {
        return ComBytes();
    }
    uint32 size_out = 0;
    uint8 buf[EVP_MAX_MD_SIZE];
    memset(buf, 0, sizeof(buf));
    if(EVP_DigestFinal_ex((EVP_MD_CTX*)ctx, buf, &size_out) != 1)
    {
        LOG_E("failed to finalize");
        return false;
    }
    ComBytes hash = ComBytes(buf, size_out);
    EVP_DigestInit_ex((EVP_MD_CTX*)ctx, (const EVP_MD*)digest, NULL);
    return hash;
}

ComBytes OpensslHash::Digest(const char* type, const void* data, int data_size)
{
    uint32 size_out = 0;
    uint8 buf[EVP_MAX_MD_SIZE];
    memset(buf, 0, sizeof(buf));
    OpenSSL_add_all_digests();
    if(EVP_Digest((const uint8*)data, data_size, buf, &size_out, EVP_get_digestbyname(type), NULL) != 1)
    {
        return ComBytes();
    }
    return ComBytes(buf, size_out);
}

OpensslHMAC::OpensslHMAC()
{
    OpenSSL_add_all_digests();
}

OpensslHMAC::~OpensslHMAC()
{
    if(ctx != NULL)
    {
        EVP_MAC_CTX_free((EVP_MAC_CTX*)ctx);
        ctx = NULL;
    }
    if(mac != NULL)
    {
        EVP_MAC_free((EVP_MAC*)mac);
        mac = NULL;
    }
}

bool OpensslHMAC::setType(const char* type)
{
    if(type == NULL)
    {
        return false;
    }
    this->type = type;
    return true;
}

bool OpensslHMAC::setKey(const char* key)
{
    return setKey((const uint8*)key, com_string_len(key));
}

bool OpensslHMAC::setKey(const uint8* key, int key_size)
{
    if(key == NULL || key_size <= 0)
    {
        return false;
    }
    this->key.append(key, key_size);
    return true;
}

bool OpensslHMAC::append(const void* data, int data_size)
{
    if(data == NULL || data_size <= 0)
    {
        return false;
    }
    if(ctx == NULL)
    {
        mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
        if(mac == NULL)
        {
            LOG_I("failed to get hamc");
            return false;
        }
        ctx = EVP_MAC_CTX_new((EVP_MAC*)mac);
        if(ctx == NULL)
        {
            LOG_E("failed to create ctx");
        }
        OSSL_PARAM params[2] =
        {
            OSSL_PARAM_utf8_string("digest", (void*)type.c_str(), type.length()),
            OSSL_PARAM_END
        };
        if(EVP_MAC_init((EVP_MAC_CTX*)ctx, key.getData(), key.getDataSize(), params) != 1)
        {
            LOG_E("failed to int hamc");
            return false;
        }
    }
    if(EVP_MAC_update((EVP_MAC_CTX*)ctx, (const uint8*)data, data_size) != 1)
    {
        LOG_E("failed to update");
        return false;
    }
    return true;
}

ComBytes OpensslHMAC::finish()
{
    if(ctx == NULL)
    {
        return ComBytes();
    }
    size_t size_out = 0;
    uint8 buf[EVP_MAX_MD_SIZE];
    memset(buf, 0, sizeof(buf));
    if(EVP_MAC_final((EVP_MAC_CTX*)ctx, buf, &size_out, sizeof(buf)) != 1)
    {
        LOG_E("failed to finalize");
        return false;
    }
    ComBytes hash = ComBytes(buf, size_out);
    if(ctx != NULL)
    {
        EVP_MAC_CTX_free((EVP_MAC_CTX*)ctx);
        ctx = NULL;
    }
    if(mac != NULL)
    {
        EVP_MAC_free((EVP_MAC*)mac);
        mac = NULL;
    }
    return hash;
}

ComBytes OpensslHMAC::Digest(const char* type, const void* data, int data_size, const void* key, int key_size)
{
    if(key_size == 0)
    {
        key_size = com_string_len((const char*)key);
    }
#if 0
    OpensslHMAC hmac;
    hmac.setType(type);
    hmac.setKey((const uint8*)key, key_size);
    hmac.append(data, data_size);
    return hmac.finish();
#else
    uint32 size_out = 0;
    uint8 buf[EVP_MAX_MD_SIZE * 2];
    memset(buf, 0, sizeof(buf));
    OpenSSL_add_all_digests();
    HMAC(EVP_get_digestbyname(type), key, key_size, (const uint8*)data, data_size, buf, &size_out);
    return ComBytes(buf, size_out);
#endif
}

OpensslCrypto::OpensslCrypto()
{
    OSSL_PROVIDER_try_load(NULL, "legacy", 1);
    //OSSL_PROVIDER_try_load(NULL, "default", 1);
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

bool OpensslCrypto::setTag(const ComBytes& tag)
{
    this->tag = tag;
    return true;
}

ComBytes OpensslCrypto::getTag()
{
    return tag;
}

ComBytes OpensslCrypto::encrypt(const uint8* data, int data_size)
{
    if(encryptBegin() == false)
    {
        return ComBytes();
    }

    ComBytes bytes;
    if(encryptAppend(bytes, data, data_size) == false)
    {
        return ComBytes();
    }

    if(encryptEnd(bytes) == false)
    {
        return ComBytes();
    }
    return bytes;
}

ComBytes OpensslCrypto::decrypt(const uint8* data, int data_size)
{
    if(decryptBegin() == false)
    {
        return ComBytes();
    }

    ComBytes bytes;
    if(decryptAppend(bytes, data, data_size) == false)
    {
        return ComBytes();
    }

    if(decryptEnd(bytes) == false)
    {
        return ComBytes();
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
    ComBytes bytes;
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
    ComBytes bytes;
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
        LOG_E("failed to get cipher:%s", engine_mode.c_str());
        return false;
    }
    block_size = EVP_CIPHER_block_size(cipher);

    if(EVP_EncryptInit_ex((EVP_CIPHER_CTX*)ctx, cipher, NULL, NULL, NULL) != 1)
    {
        LOG_E("failed to int cipher:%s,error=%s", engine_mode.c_str(), ERR_error_string(ERR_get_error(), NULL));
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

bool OpensslCrypto::encryptAppend(ComBytes& result, const uint8* data, int data_size)
{
    if(ctx == NULL || data == NULL || data_size <= 0)
    {
        LOG_E("arg incorrect,ctx=%p,data=%p,data_size=%d", ctx, data, data_size);
        return false;
    }
    int size_out = 0;
    uint8* buf = new uint8[data_size + block_size];
    if(EVP_EncryptUpdate((EVP_CIPHER_CTX*)ctx, buf, &size_out, data, data_size) != 1)
    {
        LOG_E("failed to update");
        return false;
    }
    result.append(buf, size_out);
    delete[] buf;
    return true;
}

bool OpensslCrypto::encryptEnd(ComBytes& result)
{
    if(ctx == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    // 结束加密操作
    int size_out = 0;
    uint8* buf = new uint8[block_size * 2];
    if(EVP_EncryptFinal_ex((EVP_CIPHER_CTX*)ctx, buf, &size_out) != 1)
    {
        LOG_W("failed to EVP_EncryptFinal_ex");
        //return false;
    }
    if(size_out > 0)
    {
        result.append(buf, size_out);
    }
    delete[] buf;

    if(engine_mode == "CCM" || engine_mode == "GCM")
    {
        uint8 buf_tag[OPENSSL_AES_TAG_SIZE];
        memset(buf_tag, 0, sizeof(buf_tag));
        if(EVP_CIPHER_CTX_ctrl((EVP_CIPHER_CTX*)ctx, engine_mode == "CCM" ? EVP_CTRL_CCM_GET_TAG : EVP_CTRL_GCM_GET_TAG, sizeof(buf_tag), buf_tag) != 1)
        {
            LOG_E("failed to get tag");
            return false;
        }
        tag = ComBytes(buf_tag, sizeof(buf_tag));
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
    block_size = EVP_CIPHER_block_size(cipher);

    if(EVP_DecryptInit_ex((EVP_CIPHER_CTX*)ctx, cipher, NULL, NULL, NULL) != 1)
    {
        LOG_E("failed to int cipher:%s,error=%s", engine_mode.c_str(), ERR_error_string(ERR_get_error(), NULL));
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

bool OpensslCrypto::decryptAppend(ComBytes& result, const uint8* data, int data_size)
{
    if(ctx == NULL || data == NULL || data_size <= 0)
    {
        LOG_E("arg incorrect,ctx=%p,data=%p,data_size=%d", ctx, data, data_size);
        return false;
    }
    int size_out = 0;
    uint8* buf = new uint8[data_size + block_size];
    if(EVP_DecryptUpdate((EVP_CIPHER_CTX*)ctx, buf, &size_out, data, data_size) != 1)
    {
        LOG_E("failed to update");
        return false;
    }
    result.append(buf, size_out);
    delete[] buf;
    return true;
}

bool OpensslCrypto::decryptEnd(ComBytes& result)
{
    if(ctx == NULL)
    {
        LOG_E("arg incorrect");
        return false;
    }
    int size_out = 0;
    uint8* buf = new uint8[block_size * 2];
    if(EVP_DecryptFinal_ex((EVP_CIPHER_CTX*)ctx, buf, &size_out) != 1)
    {
        LOG_W("failed,size_out=%d", size_out);
        return false;
    }
    if(size_out > 0)
    {
        result.append(buf, size_out);
    }
    delete[] buf;
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

OpensslSM4::OpensslSM4()
{
}

OpensslSM4::~OpensslSM4()
{
}

const void* OpensslSM4::getCipher()
{
    if(key.getDataSize() != 16)
    {
        LOG_E("%s key size incorrect:%d,must be 16B", engine_mode.c_str(), key.getDataSize());
        return NULL;
    }
    if(engine_mode == "ECB")
    {
        return EVP_sm4_ecb();
    }
    else if(engine_mode == "CBC")
    {
        return EVP_sm4_cbc();
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
    key_pub = NULL;
    key_pri = NULL;
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
    OSSL_DECODER_CTX* dctx = OSSL_DECODER_CTX_new_for_pkey((EVP_PKEY**)&key_pub,
                             "PEM", NULL, "RSA", OSSL_KEYMGMT_SELECT_PUBLIC_KEY, NULL, NULL);
    if(dctx == NULL)
    {
        BIO_free(b);
        LOG_E("failed to decode key");
        return false;
    }
    if(pwd != NULL)
    {
        OSSL_DECODER_CTX_set_passphrase(dctx, (const uint8*)pwd, strlen(pwd));
    }
    OSSL_DECODER_from_bio(dctx, b);
    OSSL_DECODER_CTX_free(dctx);
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
    OSSL_DECODER_CTX* dctx = OSSL_DECODER_CTX_new_for_pkey((EVP_PKEY**)&key_pri,
                             "PEM", NULL, "RSA", OSSL_KEYMGMT_SELECT_PRIVATE_KEY, NULL, NULL);
    if(dctx == NULL)
    {
        LOG_E("failed to decode key");
        return false;
    }
    OSSL_DECODER_CTX_set_passphrase(dctx, (const uint8*)pwd, strlen(pwd));
    OSSL_DECODER_from_bio(dctx, b);
    OSSL_DECODER_CTX_free(dctx);
    BIO_free(b);
    return true;
}

bool OpensslRSA::setPublicKeyFromFile(const char* file_public_key, const char* pwd)
{
    if(file_public_key == NULL)
    {
        return false;
    }
    ComBytes content = com_file_readall(file_public_key);
    return setPublicKey(content.toString().c_str(), pwd);
}

bool OpensslRSA::setPrivateKeyFromFile(const char* file_private_key, const char* pwd)
{
    if(file_private_key == NULL)
    {
        return false;
    }
    ComBytes content = com_file_readall(file_public_key);
    return setPrivateKey(content.toString().c_str(), pwd);
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

ComBytes OpensslRSA::encryptWithPublicKey(uint8* data, int data_size)
{
    if(data == NULL || data_size <= 0 || key_pub == NULL)
    {
        LOG_E("arg incorrect,data=%p,data_size=%d,key_pub=%p", data, data_size, key_pub);
        return ComBytes();
    }
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new((EVP_PKEY*)key_pub, NULL);
    EVP_PKEY_encrypt_init(ctx);
    EVP_PKEY_CTX_set_rsa_padding(ctx, padding_mode);

    size_t out_size = 0;
    if(EVP_PKEY_encrypt(ctx, NULL, &out_size, data, data_size) != 1)
    {
        EVP_PKEY_CTX_free(ctx);
        LOG_E("failed to get encrypt data size");
        return ComBytes();
    }
    std::vector<uint8> buf;
    buf.reserve(out_size);
    int ret = EVP_PKEY_encrypt(ctx, buf.data(), &out_size, data, data_size);
    EVP_PKEY_CTX_free(ctx);
    if(ret != 1)
    {
        LOG_E("rsa public encrypt failed,ret=%d,err=0x%lX", ret, ERR_get_error());
        return ComBytes();
    }
    return ComBytes(buf.data(), out_size);
}

ComBytes OpensslRSA::decryptWithPrivateKey(uint8* data, int data_size)
{
    if(data == NULL || data_size <= 0 || key_pri == NULL)
    {
        return ComBytes();
    }
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new((EVP_PKEY*)key_pri, NULL);
    EVP_PKEY_decrypt_init(ctx);
    EVP_PKEY_CTX_set_rsa_padding(ctx, padding_mode);

    size_t out_size = 0;
    if(EVP_PKEY_decrypt(ctx, NULL, &out_size, data, data_size) != 1)
    {
        EVP_PKEY_CTX_free(ctx);
        LOG_E("failed to get decrypt data size");
        return ComBytes();
    }
    std::vector<uint8> buf;
    buf.reserve(out_size);
    int ret = EVP_PKEY_decrypt(ctx, buf.data(), &out_size, data, data_size);
    EVP_PKEY_CTX_free(ctx);
    if(ret != 1)
    {
        LOG_E("rsa private decrypt failed,ret=%d,err=0x%lX", ret, ERR_get_error());
        return ComBytes();
    }
    return ComBytes(buf.data(), out_size);
}

ComBytes OpensslRSA::encryptWithPrivateKey(uint8* data, int data_size)
{
#if 0
    if(data == NULL || data_size <= 0 || key_pri == NULL)
    {
        return ComBytes();
    }
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new((EVP_PKEY*)key_pri, NULL);
    EVP_PKEY_encrypt_init(ctx);
    EVP_PKEY_CTX_set_rsa_padding(ctx, padding_mode);

    size_t out_size = 0;
    if(EVP_PKEY_encrypt(ctx, NULL, &out_size, data, data_size) != 1)
    {
        EVP_PKEY_CTX_free(ctx);
        LOG_E("failed to get encrypt data size");
        return ComBytes();
    }
    LOG_I("encrypt data size=%zu", out_size);
    std::vector<uint8> buf;
    buf.reserve(out_size);
    int ret = EVP_PKEY_encrypt(ctx, buf.data(), &out_size, data, data_size);
    EVP_PKEY_CTX_free(ctx);
    if(ret != 1)
    {
        LOG_E("rsa public encrypt failed,ret=%d,err=0x%lX", ret, ERR_get_error());
        return ComBytes();
    }
    return ComBytes(buf.data(), out_size);
#else
    LOG_E("not support in openssl3");
    return ComBytes();
#endif
}

ComBytes OpensslRSA::decryptWithPublicKey(uint8* data, int data_size)
{
#if 0
    if(data == NULL || data_size <= 0 || key_pub == NULL)
    {
        return ComBytes();
    }
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new((EVP_PKEY*)key_pub, NULL);
    EVP_PKEY_decrypt_init(ctx);
    EVP_PKEY_CTX_set_rsa_padding(ctx, padding_mode);

    size_t out_size = 0;
    if(EVP_PKEY_decrypt(ctx, NULL, &out_size, data, data_size) != 1)
    {
        EVP_PKEY_CTX_free(ctx);
        LOG_E("failed to get decrypt data size");
        return ComBytes();
    }
    LOG_I("decrypt data size=%zu", out_size);
    std::vector<uint8> buf;
    buf.reserve(out_size);
    int ret = EVP_PKEY_decrypt(ctx, buf.data(), &out_size, data, data_size);
    EVP_PKEY_CTX_free(ctx);
    if(ret != 1)
    {
        LOG_E("rsa public decrypt failed,ret=%d,err=%s", ret, ERR_error_string(ERR_get_error(), NULL));
        return ComBytes();
    }
    return ComBytes(buf.data(), out_size);
#else
    LOG_E("not support in openssl3");
    return ComBytes();
#endif
}

void OpensslRSA::cleanPublicKey()
{
    if(key_pub != NULL)
    {
        EVP_PKEY_free((EVP_PKEY*)key_pub);
        key_pub = NULL;
    }
}

void OpensslRSA::cleanPrivateKey()
{
    if(key_pri != NULL)
    {
        EVP_PKEY_free((EVP_PKEY*)key_pri);
        key_pri = NULL;
    }
}

bool OpensslRSA::GenerateKey(int key_bits, std::string& public_key, std::string& private_key, const char* pwd)
{
    BIO* b_pub = BIO_new(BIO_s_mem());
    if(b_pub == NULL)
    {
        return false;
    }
    BIO* b_pri = BIO_new(BIO_s_mem());
    if(b_pri == NULL)
    {
        BIO_free_all(b_pub);
        return false;
    }

    EVP_PKEY* key = NULL;
    EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);

    EVP_PKEY_keygen_init(pctx);
    EVP_PKEY_CTX_set_rsa_keygen_bits(pctx, key_bits);
    EVP_PKEY_generate(pctx, &key);

    PEM_write_bio_PUBKEY(b_pub, key);
    PEM_write_bio_PrivateKey(b_pri, key, NULL, NULL, 0, pwd == NULL ? NULL : openssl_password_cb, (void*)pwd);

    char* data_pri = NULL;
    char* data_pub = NULL;
    int size_pri = BIO_get_mem_data(b_pri, &data_pri);
    int size_pub = BIO_get_mem_data(b_pub, &data_pub);
    if(size_pub <= 0 || data_pub == NULL || size_pri <= 0 || data_pri == NULL)
    {
        BIO_free_all(b_pub);
        BIO_free_all(b_pri);
        EVP_PKEY_CTX_free(pctx);
        return false;
    }
    private_key = std::string(data_pri, size_pri);
    public_key = std::string(data_pub, size_pub);

    BIO_free_all(b_pub);
    BIO_free_all(b_pri);
    EVP_PKEY_CTX_free(pctx);
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
    ComBytes bytes = com_file_readall(file);
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
    X509* x509 = NULL;
    PKCS12* p12 = NULL;
    bool result = false;

    OpenSSL_add_all_digests();

    do
    {
        EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);

        EVP_PKEY_keygen_init(pctx);
        EVP_PKEY_CTX_set_rsa_keygen_bits(pctx, params.getInt32("key_bits", 2048));
        EVP_PKEY_generate(pctx, &key);
        EVP_PKEY_CTX_free(pctx);

        if(params.isKeyExist("file_private_key"))
        {
            std::string pwd = params.getString("password_private_key");
            std::string file =  params.getString("file_private_key");
            FILE* f = com_file_open(file.c_str(), "wb+");
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
                if(com_string_is_ipv4(vals[i].c_str()))
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
            FILE* f = com_file_open(file.c_str(), "wb+");
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
            FILE* f = com_file_open(file.c_str(), "wb+");
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
    return result;
}

