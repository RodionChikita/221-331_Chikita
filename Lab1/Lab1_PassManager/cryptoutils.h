#ifndef CRYPTOUTILS_H
#define CRYPTOUTILS_H

#include <QByteArray>
#include <QString>

namespace CryptoUtils {

    QByteArray deriveKey(const QString &pin, const QByteArray &salt);

    QByteArray decryptAes256Cbc(const QByteArray &ciphertext,
                                const QByteArray &key,
                                const QByteArray &iv);

    QByteArray encryptAes256Cbc(const QByteArray &plaintext,
                                const QByteArray &key,
                                const QByteArray &iv);

    QByteArray decryptCredentialsFile(const QString &filePath,
                                      const QString &pin);

    QByteArray encryptField(const QByteArray &plaintext, const QString &pin);
    QByteArray decryptField(const QByteArray &ciphertext, const QString &pin);

}

#endif
