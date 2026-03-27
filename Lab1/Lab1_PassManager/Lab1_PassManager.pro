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

    isEmpty(OPENSSL_DIR) {
        for(p, $$list(C:/Qt/Tools/OpenSSLv3/Win_x64 C:/OpenSSL-Win64)) {
            exists($$p/include/openssl/evp.h) {
                OPENSSL_DIR = $$p
                break()
            }
        }
    }

    !isEmpty(OPENSSL_DIR) {
        INCLUDEPATH += $$OPENSSL_DIR/include

        # MinGW: .lib files from MSVC OpenSSL don't work, link to DLL directly
        mingw {
            LIBS += -L$$OPENSSL_DIR/lib -lcrypto.dll
        } else {
            LIBS += -L$$OPENSSL_DIR/lib -llibcrypto
        }
    }

    LIBS += -ladvapi32
}

unix {
    LIBS += -lcrypto
}
