cmake_minimum_required(VERSION 3.16)
project(Lab1_PassManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# --- OpenSSL ---
# Поиск: 1) переменная OPENSSL_ROOT_DIR  2) стандартные пути
if(NOT OPENSSL_ROOT_DIR)
    if(DEFINED ENV{OPENSSL_DIR})
        set(OPENSSL_ROOT_DIR "$ENV{OPENSSL_DIR}")
    endif()
endif()

find_package(OpenSSL QUIET)

if(NOT OpenSSL_FOUND AND OPENSSL_ROOT_DIR)
    # Для установок без .cmake/.pc (slproweb и т.п.) — ручной поиск
    find_path(OPENSSL_INCLUDE_DIR openssl/evp.h PATHS "${OPENSSL_ROOT_DIR}/include" NO_DEFAULT_PATH)
    find_library(OPENSSL_CRYPTO_LIBRARY NAMES crypto libcrypto PATHS "${OPENSSL_ROOT_DIR}/lib" NO_DEFAULT_PATH)
    find_file(OPENSSL_CRYPTO_DLL NAMES libcrypto-3-x64.dll libcrypto-3.dll PATHS "${OPENSSL_ROOT_DIR}/bin" NO_DEFAULT_PATH)

    if(OPENSSL_INCLUDE_DIR AND (OPENSSL_CRYPTO_LIBRARY OR OPENSSL_CRYPTO_DLL))
        add_library(OpenSSL::Crypto SHARED IMPORTED)
        if(OPENSSL_CRYPTO_LIBRARY)
            set_target_properties(OpenSSL::Crypto PROPERTIES
                IMPORTED_IMPLIB "${OPENSSL_CRYPTO_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")
        endif()
        if(OPENSSL_CRYPTO_DLL)
            set_target_properties(OpenSSL::Crypto PROPERTIES
                IMPORTED_LOCATION "${OPENSSL_CRYPTO_DLL}"
                INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")
        endif()
        set(OpenSSL_FOUND TRUE)
        message(STATUS "OpenSSL found manually: ${OPENSSL_ROOT_DIR}")
    endif()
endif()

if(NOT OpenSSL_FOUND)
    message(FATAL_ERROR
        "OpenSSL not found. Set OPENSSL_ROOT_DIR or OPENSSL_DIR environment variable.\n"
        "  cmake -DOPENSSL_ROOT_DIR=C:/OpenSSL-Win64 ..")
endif()

# --- Sources ---
set(SOURCES
    main.cpp
    loginwindow.cpp
    credentialswindow.cpp
    cryptoutils.cpp
    integritycheck.cpp
    antidebug.cpp
)

set(HEADERS
    loginwindow.h
    credentialswindow.h
    cryptoutils.h
    integritycheck.h
    antidebug.h
)

add_executable(Lab1_PassManager WIN32 ${SOURCES} ${HEADERS})

target_link_libraries(Lab1_PassManager PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    OpenSSL::Crypto
)

if(WIN32)
    target_link_libraries(Lab1_PassManager PRIVATE advapi32)
endif()
