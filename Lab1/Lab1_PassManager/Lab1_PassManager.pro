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

# OpenSSL
win32 {
    isEmpty(OPENSSL_DIR): OPENSSL_DIR = $$(OPENSSL_DIR)

    !isEmpty(OPENSSL_DIR) {
        INCLUDEPATH += $$OPENSSL_DIR/include
        LIBS += -L$$OPENSSL_DIR/lib -lcrypto
    } else {
        for(p, $$list(C:/Qt/Tools/OpenSSLv3/Win_x64 C:/OpenSSL-Win64)) {
            exists($$p/include/openssl/evp.h) {
                INCLUDEPATH += $$p/include
                LIBS += -L$$p/lib -lcrypto
                break()
            }
        }
    }

    LIBS += -ladvapi32
}

unix {
    LIBS += -lcrypto
}
