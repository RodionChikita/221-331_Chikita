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

# --- Автоматическое развёртывание: после сборки рядом с exe кладём Qt DLL и OpenSSL,
# чтобы программа запускалась двойным кликом без Qt в PATH ---
if(WIN32)
    get_target_property(_qt_qmake Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin "${_qt_qmake}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin}" NO_DEFAULT_PATH)
    if(NOT WINDEPLOYQT_EXECUTABLE)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt)
    endif()

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET Lab1_PassManager POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-system-d3d-compiler
                    --no-opengl-sw --no-quick-import "$<TARGET_FILE:Lab1_PassManager>"
            COMMENT "windeployqt: копирование Qt DLL рядом с exe")
    else()
        message(WARNING "windeployqt не найден: Qt DLL придётся копировать вручную")
    endif()

    # OpenSSL DLL (libcrypto) рядом с exe
    set(_ossl_hints "${OPENSSL_ROOT_DIR}/bin" "$ENV{OPENSSL_DIR}/bin")
    if(OPENSSL_CRYPTO_LIBRARY)
        get_filename_component(_ossl_lib_dir "${OPENSSL_CRYPTO_LIBRARY}" DIRECTORY)
        list(APPEND _ossl_hints "${_ossl_lib_dir}/../bin" "${_ossl_lib_dir}")
    endif()
    find_file(_ossl_dll NAMES libcrypto-3-x64.dll libcrypto-3.dll HINTS ${_ossl_hints})
    if(_ossl_dll)
        add_custom_command(TARGET Lab1_PassManager POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_ossl_dll}" "$<TARGET_FILE_DIR:Lab1_PassManager>"
            COMMENT "Копирование libcrypto рядом с exe")
    endif()
endif()
