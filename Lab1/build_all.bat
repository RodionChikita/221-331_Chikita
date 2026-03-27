@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ============================================================
echo   Lab1 — Полная сборка и развёртывание
echo ============================================================
echo.

set "PIN=1234"
set "OUTPUT_DIR=%~dp0dist"

REM ============================================================
REM  1. Поиск Qt
REM ============================================================
echo [1/7] Поиск Qt...

if defined QTDIR if exist "%QTDIR%\bin\qmake.exe" (
    echo        Найден через QTDIR: %QTDIR%
    goto :qt_found
)

for %%V in (6.11.0 6.10.0 6.9.0 6.8.2 6.8.1 6.8.0 6.7.3 6.7.2 6.7.0 6.6.3 6.6.0 6.5.3) do (
    for %%C in (mingw_64 msvc2022_64 msvc2019_64) do (
        for %%D in (C D E) do (
            if exist "%%D:\Qt\%%V\%%C\bin\qmake.exe" (
                set "QTDIR=%%D:\Qt\%%V\%%C"
                echo        Найден: !QTDIR!
                goto :qt_found
            )
        )
    )
)

echo [ОШИБКА] Qt не найден. Установите Qt 6 или задайте QTDIR вручную:
echo          set QTDIR=C:\Qt\6.11.0\mingw_64
echo          build_all.bat
exit /b 1

:qt_found
set "PATH=%QTDIR%\bin;%PATH%"

REM Определяем MinGW или MSVC
echo "%QTDIR%" | findstr /i "mingw" >nul
if %errorlevel%==0 (
    set "USE_MINGW=1"
    echo        Компилятор: MinGW

    REM Добавляем MinGW tools в PATH
    for %%D in (C D E) do (
        for /d %%G in ("%%D:\Qt\Tools\mingw*") do (
            if exist "%%G\bin\g++.exe" (
                set "PATH=%%G\bin;!PATH!"
                echo        MinGW tools: %%G
                goto :mingw_found
            )
        )
    )
    :mingw_found
) else (
    set "USE_MINGW=0"
    echo        Компилятор: MSVC
)

REM ============================================================
REM  2. Поиск OpenSSL
REM ============================================================
echo [2/7] Поиск OpenSSL...

if defined OPENSSL_DIR if exist "%OPENSSL_DIR%\include\openssl\evp.h" (
    echo        Найден через OPENSSL_DIR: %OPENSSL_DIR%
    goto :ssl_found
)

for %%D in (C D E) do (
    for %%P in (
        "%%D:\Qt\Tools\OpenSSLv3\Win_x64"
        "%%D:\OpenSSL-Win64"
        "%%D:\Program Files\OpenSSL-Win64"
        "%%D:\OpenSSL\Win_x64"
    ) do (
        if exist "%%~P\include\openssl\evp.h" (
            set "OPENSSL_DIR=%%~P"
            echo        Найден: !OPENSSL_DIR!
            goto :ssl_found
        )
    )
)

echo [ОШИБКА] OpenSSL не найден. Установите OpenSSL 3.x или задайте OPENSSL_DIR:
echo          set OPENSSL_DIR=C:\Qt\Tools\OpenSSLv3\Win_x64
echo          build_all.bat
exit /b 1

:ssl_found

REM ============================================================
REM  3. Проверка компилятора
REM ============================================================
echo [3/7] Проверка компилятора...

if "%USE_MINGW%"=="1" (
    where g++ >nul 2>&1
    if errorlevel 1 (
        echo [ОШИБКА] g++.exe не найден в PATH.
        echo          Убедитесь, что Qt MinGW tools установлены через Qt Maintenance Tool.
        exit /b 1
    )
    echo        g++.exe — OK
    set "MAKE_CMD=mingw32-make"
) else (
    where cl >nul 2>&1
    if errorlevel 1 (
        echo [ОШИБКА] cl.exe не найден. Запустите из "x64 Native Tools Command Prompt for VS 2022"
        exit /b 1
    )
    echo        cl.exe — OK
    set "MAKE_CMD=nmake"
)

REM ============================================================
REM  4. Шифрование файла данных
REM ============================================================
echo [4/7] Подготовка зашифрованного файла данных (PIN=%PIN%)...

cd /d "%~dp0Lab1_PassManager"

if not exist "credentials.json.enc" (
    where python >nul 2>&1
    if errorlevel 1 (
        echo [ОШИБКА] Python не найден. Установите Python 3 и добавьте в PATH.
        exit /b 1
    )
    pip install cryptography -q 2>nul
    python encrypt_credentials.py %PIN%
    if errorlevel 1 (
        echo [ОШИБКА] Не удалось зашифровать файл данных.
        exit /b 1
    )
) else (
    echo        credentials.json.enc уже существует, пропускаю.
    echo        Чтобы пересоздать — удалите файл и запустите снова.
)

REM ============================================================
REM  5. Сборка PassManager
REM ============================================================
echo [5/7] Сборка Lab1_PassManager...

if exist Makefile %MAKE_CMD% distclean >nul 2>&1

if "%USE_MINGW%"=="1" (
    qmake Lab1_PassManager.pro "OPENSSL_DIR=%OPENSSL_DIR%" -spec win32-g++
) else (
    qmake Lab1_PassManager.pro "OPENSSL_DIR=%OPENSSL_DIR%"
)
if errorlevel 1 (
    echo [ОШИБКА] qmake провалился. Проверьте .pro файл.
    exit /b 1
)
%MAKE_CMD% release
if errorlevel 1 (
    echo [ОШИБКА] Сборка Lab1_PassManager провалилась.
    exit /b 1
)
echo        Lab1_PassManager.exe — собран

REM ============================================================
REM  6. Сборка Protector
REM ============================================================
echo [6/7] Сборка Lab1_Protector...

cd /d "%~dp0Lab1_Protector"

if exist Makefile %MAKE_CMD% distclean >nul 2>&1

if "%USE_MINGW%"=="1" (
    qmake Lab1_Protector.pro -spec win32-g++
) else (
    qmake Lab1_Protector.pro
)
if errorlevel 1 (
    echo [ОШИБКА] qmake провалился для Protector.
    exit /b 1
)
%MAKE_CMD% release
if errorlevel 1 (
    echo [ОШИБКА] Сборка Lab1_Protector провалилась.
    exit /b 1
)
echo        Lab1_Protector.exe — собран

REM ============================================================
REM  7. Развёртывание в dist/
REM ============================================================
echo [7/7] Развёртывание в %OUTPUT_DIR%...

if exist "%OUTPUT_DIR%" rmdir /s /q "%OUTPUT_DIR%"
mkdir "%OUTPUT_DIR%"

REM Копируем exe
copy /Y "%~dp0Lab1_PassManager\release\Lab1_PassManager.exe" "%OUTPUT_DIR%\" >nul
copy /Y "%~dp0Lab1_Protector\release\Lab1_Protector.exe" "%OUTPUT_DIR%\" >nul

REM Копируем зашифрованный файл данных
copy /Y "%~dp0Lab1_PassManager\credentials.json.enc" "%OUTPUT_DIR%\" >nul

REM windeployqt — копирует все нужные Qt DLL (и MinGW runtime)
"%QTDIR%\bin\windeployqt.exe" --release --no-translations --no-opengl-sw "%OUTPUT_DIR%\Lab1_PassManager.exe" >nul

REM OpenSSL DLL
if exist "%OPENSSL_DIR%\bin\libcrypto-3-x64.dll" (
    copy /Y "%OPENSSL_DIR%\bin\libcrypto-3-x64.dll" "%OUTPUT_DIR%\" >nul
    copy /Y "%OPENSSL_DIR%\bin\libssl-3-x64.dll" "%OUTPUT_DIR%\" >nul
) else if exist "%OPENSSL_DIR%\bin\libcrypto-3.dll" (
    copy /Y "%OPENSSL_DIR%\bin\libcrypto-3.dll" "%OUTPUT_DIR%\" >nul
    copy /Y "%OPENSSL_DIR%\bin\libssl-3.dll" "%OUTPUT_DIR%\" >nul
) else if exist "%OPENSSL_DIR%\libcrypto-3-x64.dll" (
    copy /Y "%OPENSSL_DIR%\libcrypto-3-x64.dll" "%OUTPUT_DIR%\" >nul
    copy /Y "%OPENSSL_DIR%\libssl-3-x64.dll" "%OUTPUT_DIR%\" >nul
)

echo.
echo ============================================================
echo   ГОТОВО!
echo ============================================================
echo.
echo   Все файлы в:  %OUTPUT_DIR%
echo   Пин-код:      %PIN%
echo.
echo   Запуск менеджера паролей:
echo     cd "%OUTPUT_DIR%"
echo     Lab1_PassManager.exe
echo.
echo   Запуск через Protector (с антиотладкой):
echo     cd "%OUTPUT_DIR%"
echo     Lab1_Protector.exe
echo.
echo ============================================================
pause
