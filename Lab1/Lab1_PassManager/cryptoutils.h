#ifndef CRYPTOUTILS_H
#define CRYPTOUTILS_H

#include <QByteArray>
#include <QString>

namespace CryptoUtils {

    // Layer 1: file decryption key from PIN
    QByteArray deriveKey(const QString &pin, const QByteArray &salt);

    QByteArray decryptAes256Cbc(const QByteArray &ciphertext,
                                const QByteArray &key,
                                const QByteArray &iv);

    QByteArray encryptAes256Cbc(const QByteArray &plaintext,
                                const QByteArray &key,
                                const QByteArray &iv);

    // Convenience: decrypt the credentials file using the PIN
    // Returns empty QByteArray on failure
    QByteArray decryptCredentialsFile(const QString &filePath,
                                      const QString &pin);

    // Layer 2: encrypt/decrypt individual login/password fields
    QByteArray encryptField(const QByteArray &plaintext, const QString &pin);
    QByteArray decryptField(const QByteArray &ciphertext, const QString &pin);

} // namespace CryptoUtils

#endif // CRYPTOUTILS_H
