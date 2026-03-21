QT       += core gui widgets

CONFIG   += c++17

TARGET = Lab1_PassManager
TEMPLATE = app

SOURCES += \
    main.cpp \
    loginwindow.cpp \
    credentialswindow.cpp \
    cryptoutils.cpp \
    integritycheck.cpp \
    antidebug.cpp

HEADERS += \
    loginwindow.h \
    credentialswindow.h \
    cryptoutils.h \
    integritycheck.h \
    antidebug.h

# OpenSSL — auto-detect from OPENSSL_DIR env var or qmake parameter
win32 {
    # 1) qmake parameter: qmake "OPENSSL_DIR=C:/Qt/Tools/OpenSSLv3/Win_x64"
    # 2) environment variable: set OPENSSL_DIR=C:\Qt\Tools\OpenSSLv3\Win_x64
    isEmpty(OPENSSL_DIR): OPENSSL_DIR = $$(OPENSSL_DIR)

    !isEmpty(OPENSSL_DIR) {
        INCLUDEPATH += $$OPENSSL_DIR/include
        LIBS += -L$$OPENSSL_DIR/lib -llibcrypto
    } else {
        # Fallback: try common paths
        for(p, $$list(C:/Qt/Tools/OpenSSLv3/Win_x64 C:/OpenSSL-Win64)) {
            exists($$p/include/openssl/evp.h) {
                INCLUDEPATH += $$p/include
                LIBS += -L$$p/lib -llibcrypto
                break()
            }
        }
    }

    LIBS += -ladvapi32
}

unix {
    LIBS += -lcrypto
}
