#include "cryptoutils.h"

#include <QFile>
#include <QDebug>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>

static const int KEY_LEN = 32;   // AES-256
static const int IV_LEN  = 16;   // AES block size
static const int PBKDF2_ITERATIONS = 100000;

// Salt used for layer-1 (file decryption) key derivation
static const QByteArray LAYER1_SALT = QByteArray::fromHex("a1b2c3d4e5f60718");

// Salt used for layer-2 (field-level) key derivation
static const QByteArray LAYER2_SALT = QByteArray::fromHex("18070605d4c3b2a1");

// Fixed IV for layer-2 field encryption (stored alongside encrypted file)
static const QByteArray LAYER2_IV = QByteArray::fromHex("00112233445566778899aabbccddeeff");

namespace CryptoUtils {

QByteArray deriveKey(const QString &pin, const QByteArray &salt)
{
    QByteArray key(KEY_LEN, '\0');
    QByteArray pinBytes = pin.toUtf8();

    int rc = PKCS5_PBKDF2_HMAC(pinBytes.constData(), pinBytes.size(),
                                reinterpret_cast<const unsigned char*>(salt.constData()),
                                salt.size(),
                                PBKDF2_ITERATIONS,
                                EVP_sha256(),
                                KEY_LEN,
                                reinterpret_cast<unsigned char*>(key.data()));
    if (rc != 1) {
        qWarning() << "PBKDF2 key derivation failed";
        return {};
    }
    return key;
}

QByteArray decryptAes256Cbc(const QByteArray &ciphertext,
                            const QByteArray &key,
                            const QByteArray &iv)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return {};

    QByteArray plaintext(ciphertext.size() + EVP_MAX_BLOCK_LENGTH, '\0');
    int outLen = 0, totalLen = 0;

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(iv.constData())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    if (EVP_DecryptUpdate(ctx,
                          reinterpret_cast<unsigned char*>(plaintext.data()),
                          &outLen,
                          reinterpret_cast<const unsigned char*>(ciphertext.constData()),
                          ciphertext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    totalLen = outLen;

    if (EVP_DecryptFinal_ex(ctx,
                            reinterpret_cast<unsigned char*>(plaintext.data()) + totalLen,
                            &outLen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    totalLen += outLen;

    EVP_CIPHER_CTX_free(ctx);
    plaintext.resize(totalLen);
    return plaintext;
}

QByteArray encryptAes256Cbc(const QByteArray &plaintext,
                            const QByteArray &key,
                            const QByteArray &iv)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return {};

    QByteArray ciphertext(plaintext.size() + EVP_MAX_BLOCK_LENGTH, '\0');
    int outLen = 0, totalLen = 0;

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(iv.constData())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    if (EVP_EncryptUpdate(ctx,
                          reinterpret_cast<unsigned char*>(ciphertext.data()),
                          &outLen,
                          reinterpret_cast<const unsigned char*>(plaintext.constData()),
                          plaintext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    totalLen = outLen;

    if (EVP_EncryptFinal_ex(ctx,
                            reinterpret_cast<unsigned char*>(ciphertext.data()) + totalLen,
                            &outLen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    totalLen += outLen;

    EVP_CIPHER_CTX_free(ctx);
    ciphertext.resize(totalLen);
    return ciphertext;
}

QByteArray decryptCredentialsFile(const QString &filePath, const QString &pin)
{
    static const int CHUNK_SIZE = 4096;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open encrypted file:" << filePath;
        return {};
    }

    if (file.size() <= IV_LEN) {
        qWarning() << "Encrypted file is too small";
        file.close();
        return {};
    }

    QByteArray iv = file.read(IV_LEN);
    if (iv.size() != IV_LEN) {
        file.close();
        return {};
    }

    QByteArray key = deriveKey(pin, LAYER1_SALT);
    if (key.isEmpty()) {
        file.close();
        return {};
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        file.close();
        return {};
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(iv.constData())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        file.close();
        return {};
    }

    QByteArray plaintext;
    QByteArray outBuf(CHUNK_SIZE + EVP_MAX_BLOCK_LENGTH, '\0');
    int outLen = 0;

    while (!file.atEnd()) {
        QByteArray chunk = file.read(CHUNK_SIZE);
        if (chunk.isEmpty()) break;

        if (EVP_DecryptUpdate(ctx,
                              reinterpret_cast<unsigned char*>(outBuf.data()),
                              &outLen,
                              reinterpret_cast<const unsigned char*>(chunk.constData()),
                              chunk.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            file.close();
            return {};
        }
        plaintext.append(outBuf.constData(), outLen);
    }
    file.close();

    if (EVP_DecryptFinal_ex(ctx,
                            reinterpret_cast<unsigned char*>(outBuf.data()),
                            &outLen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    plaintext.append(outBuf.constData(), outLen);

    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}

QByteArray encryptField(const QByteArray &plaintext, const QString &pin)
{
    QByteArray key = deriveKey(pin, LAYER2_SALT);
    if (key.isEmpty()) return {};
    return encryptAes256Cbc(plaintext, key, LAYER2_IV);
}

QByteArray decryptField(const QByteArray &ciphertext, const QString &pin)
{
    QByteArray key = deriveKey(pin, LAYER2_SALT);
    if (key.isEmpty()) return {};
    return decryptAes256Cbc(ciphertext, key, LAYER2_IV);
}

} // namespace CryptoUtils
