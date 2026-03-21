#include "integritycheck.h"

#include <QCryptographicHash>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>

typedef quint64 QWORD;
#endif

// Reference hash — must be updated after final release build.
// Build once -> read hash from debug output -> paste here -> rebuild.
static const QByteArray REFERENCE_HASH_BASE64 =
    QByteArray("PLACEHOLDER_HASH_UPDATE_AFTER_BUILD");

namespace IntegrityCheck {

QByteArray calculateTextSegmentHash()
{
#ifdef Q_OS_WIN
    QWORD imageBase = (QWORD)GetModuleHandle(NULL);
    QWORD baseOfCode = 0x1000;
    QWORD textBase = imageBase + baseOfCode;

    PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(imageBase);
    PIMAGE_NT_HEADERS peHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(
        imageBase + dosHeader->e_lfanew);
    QWORD textSize = peHeader->OptionalHeader.SizeOfCode;

    QByteArray textSegmentContents = QByteArray(
        reinterpret_cast<const char*>(textBase), static_cast<int>(textSize));
    QByteArray hash = QCryptographicHash::hash(
        textSegmentContents, QCryptographicHash::Sha256);
    QByteArray hashBase64 = hash.toBase64();

    qDebug() << "textBase =" << Qt::hex << textBase;
    qDebug() << "textSize =" << textSize;
    qDebug() << "calculatedTextHashBase64 =" << hashBase64;

    return hashBase64;
#else
    return {};
#endif
}

bool verify()
{
#ifdef Q_OS_WIN
    QByteArray current = calculateTextSegmentHash();

    if (REFERENCE_HASH_BASE64 == "PLACEHOLDER_HASH_UPDATE_AFTER_BUILD") {
        qDebug() << "IntegrityCheck: reference hash not set, skipping check";
        return true;
    }

    bool ok = (current == REFERENCE_HASH_BASE64);
    qDebug() << "IntegrityCheck result =" << ok;
    return ok;
#else
    return true;
#endif
}

} // namespace IntegrityCheck
