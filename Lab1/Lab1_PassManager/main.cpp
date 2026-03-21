#include <QApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QDir>
#include <QDebug>

#include "loginwindow.h"
#include "credentialswindow.h"
#include "cryptoutils.h"
#include "antidebug.h"
#include "integritycheck.h"

static QString encryptedFilePath()
{
    return QApplication::applicationDirPath() + "/credentials.json.enc";
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    LoginWindow loginWindow;
    CredentialsWindow credentialsWindow;

    // --- Anti-debug: IsDebuggerPresent ---
    // Uncomment the block below for the defense demonstration.
    // Comment it out when testing the Protector (self-debugging) approach.
    /*
    if (AntiDebug::isDebuggerAttached()) {
        loginWindow.show();
        loginWindow.showAttackWarning(
            "ВНИМАНИЕ: Обнаружен отладчик!\n"
            "Работа приложения заблокирована.");
        return app.exec();
    }
    */

    // --- Integrity check: .text segment hash ---
    if (!IntegrityCheck::verify()) {
        loginWindow.show();
        loginWindow.showAttackWarning(
            "ВНИМАНИЕ: Обнаружена модификация исполняемого файла!\n"
            "Контрольная сумма сегмента .text не совпадает с эталоном.\n"
            "Работа приложения заблокирована.");
        return app.exec();
    }

    // --- Normal startup ---
    loginWindow.show();

    QObject::connect(&loginWindow, &LoginWindow::pinAccepted,
                     [&](const QString &pin) {
        QString filePath = encryptedFilePath();

        QByteArray decryptedJson = CryptoUtils::decryptCredentialsFile(filePath, pin);
        if (decryptedJson.isEmpty()) {
            QMessageBox::warning(&loginWindow, "Ошибка",
                                 "Неверный пин-код или файл повреждён.");
            return;
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(decryptedJson, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
            QMessageBox::warning(&loginWindow, "Ошибка",
                                 "Неверный пин-код (ошибка парсинга JSON).");
            return;
        }

        QJsonArray arr = doc.array();
        QVector<Credential> credentials;
        credentials.reserve(arr.size());

        for (const QJsonValue &val : arr) {
            QJsonObject obj = val.toObject();
            Credential cred;
            cred.url = obj["url"].toString();

            // Layer 2: login and password remain encrypted in memory.
            // The JSON stores them as base64-encoded ciphertexts.
            cred.encryptedLogin = QByteArray::fromBase64(
                obj["encrypted_login"].toString().toLatin1());
            cred.encryptedPassword = QByteArray::fromBase64(
                obj["encrypted_password"].toString().toLatin1());

            credentials.append(cred);
        }

        credentialsWindow.setCredentials(credentials);
        loginWindow.hide();
        credentialsWindow.show();
    });

    QObject::connect(&credentialsWindow, &CredentialsWindow::logoutRequested,
                     [&]() {
        credentialsWindow.hide();
        loginWindow.show();
    });

    return app.exec();
}
