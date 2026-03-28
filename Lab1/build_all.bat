@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ============================================================
echo   Lab1 — Сборка через CMake и развёртывание
echo ============================================================
echo.

set "PIN=1234"
set "OUTPUT_DIR=%~dp0dist"
set "BUILD_DIR=%~dp0build"

REM ============================================================
REM  1. Поиск Qt
REM ============================================================
echo [1/7] Поиск Qt...

if defined QTDIR if exist "%QTDIR%\bin\qt-cmake.bat" (
    echo        Найден через QTDIR: %QTDIR%
    goto :qt_found
)
if defined QTDIR if exist "%QTDIR%\bin\qmake.exe" (
    echo        Найден через QTDIR: %QTDIR%
    goto :qt_found
)

for %%V in (6.11.0 6.10.0 6.9.0 6.8.2 6.8.1 6.8.0 6.7.3 6.7.2 6.7.0 6.6.3 6.6.0 6.5.3) do (
    for %%C in (mingw_64 msvc2022_64 msvc2019_64) do (
        for %%D in (C D E) do (
            if exist "%%D:\Qt\%%V\%%C\lib\cmake\Qt6\Qt6Config.cmake" (
                set "QTDIR=%%D:\Qt\%%V\%%C"
                echo        Найден: !QTDIR!
                goto :qt_found
            )
        )
    )
)

echo [ОШИБКА] Qt не найден. Задайте QTDIR:
echo          set QTDIR=C:\Qt\6.11.0\mingw_64
echo          build_all.bat
exit /b 1

:qt_found
set "CMAKE_PREFIX_PATH=!QTDIR!"
set "PATH=%QTDIR%\bin;%PATH%"

REM ============================================================
REM  Определяем MinGW или MSVC
REM ============================================================
set "USE_MINGW=0"
echo !QTDIR! | findstr /i "mingw" >nul && set "USE_MINGW=1"

if "!USE_MINGW!"=="1" (
    echo        Компилятор: MinGW
) else (
    echo        Компилятор: MSVC
)

if "!USE_MINGW!"=="0" goto :skip_mingw_search

for %%D in (C D E) do (
    if exist "%%D:\Qt\Tools" (
        for /d %%G in ("%%D:\Qt\Tools\mingw*") do (
            if exist "%%~G\bin\g++.exe" (
                set "PATH=%%~G\bin;!PATH!"
                echo        MinGW tools: %%~G
                goto :skip_mingw_search
            )
        )
    )
)

:skip_mingw_search

REM ============================================================
REM  2. Поиск OpenSSL
REM ============================================================
echo [2/7] Поиск OpenSSL...

if defined OPENSSL_DIR if exist "%OPENSSL_DIR%\include\openssl\evp.h" (
    echo        Найден через OPENSSL_DIR: %OPENSSL_DIR%
    goto :ssl_found
)

for %%D in (C D E) do (
    if exist "%%D:\Qt\Tools\OpenSSLv3\Win_x64\include\openssl\evp.h" (
        set "OPENSSL_DIR=%%D:\Qt\Tools\OpenSSLv3\Win_x64"
        echo        Найден: !OPENSSL_DIR!
        goto :ssl_found
    )
    if exist "%%D:\OpenSSL-Win64\include\openssl\evp.h" (
        set "OPENSSL_DIR=%%D:\OpenSSL-Win64"
        echo        Найден: !OPENSSL_DIR!
        goto :ssl_found
    )
    if exist "%%D:\Program Files\OpenSSL-Win64\include\openssl\evp.h" (
        set "OPENSSL_DIR=%%D:\Program Files\OpenSSL-Win64"
        echo        Найден: !OPENSSL_DIR!
        goto :ssl_found
    )
)

echo [ОШИБКА] OpenSSL не найден. Задайте OPENSSL_DIR:
echo          set OPENSSL_DIR=C:\OpenSSL-Win64
echo          build_all.bat
exit /b 1

:ssl_found

REM ============================================================
REM  3. Проверка cmake
REM ============================================================
echo [3/7] Проверка инструментов...

where cmake >nul 2>&1
if errorlevel 1 (
    if exist "!QTDIR!\bin\qt-cmake.bat" (
        set "CMAKE_CMD=!QTDIR!\bin\qt-cmake.bat"
    ) else (
        echo [ОШИБКА] cmake не найден. Установите CMake и добавьте в PATH.
        exit /b 1
    )
) else (
    set "CMAKE_CMD=cmake"
)
echo        cmake — OK

if "!USE_MINGW!"=="1" (
    where g++ >nul 2>&1
    if errorlevel 1 (
        echo [ОШИБКА] g++.exe не найден в PATH.
        exit /b 1
    )
    echo        g++.exe — OK
    set "CMAKE_GENERATOR=-G "MinGW Makefiles""
) else (
    set "CMAKE_GENERATOR="
)

REM ============================================================
REM  4. Шифрование файла данных
REM ============================================================
echo [4/7] Подготовка зашифрованного файла данных (PIN=%PIN%)...

cd /d "%~dp0Lab1_PassManager"

if exist "credentials.json.enc" (
    echo        credentials.json.enc уже существует, пропускаю.
    goto :encrypt_done
)

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

:encrypt_done

REM ============================================================
REM  5. Сборка CMake (оба проекта)
REM ============================================================
echo [5/7] CMake configure...

cd /d "%~dp0"

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

!CMAKE_CMD! -S . -B "%BUILD_DIR%" !CMAKE_GENERATOR! -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="!QTDIR!" -DOPENSSL_ROOT_DIR="!OPENSSL_DIR!"
if errorlevel 1 (
    echo [ОШИБКА] CMake configure провалился.
    exit /b 1
)

echo [6/7] CMake build...

!CMAKE_CMD! --build "%BUILD_DIR%" --config Release
if errorlevel 1 (
    echo [ОШИБКА] CMake build провалился.
    exit /b 1
)
echo        Lab1_PassManager.exe — собран
echo        Lab1_Protector.exe — собран

REM ============================================================
REM  7. Развёртывание в dist/
REM ============================================================
echo [7/7] Развёртывание в %OUTPUT_DIR%...

if exist "%OUTPUT_DIR%" rmdir /s /q "%OUTPUT_DIR%"
mkdir "%OUTPUT_DIR%"

REM Ищем exe в build/ (структура зависит от генератора)
set "PM_EXE="
if exist "%BUILD_DIR%\Lab1_PassManager\Lab1_PassManager.exe" set "PM_EXE=%BUILD_DIR%\Lab1_PassManager\Lab1_PassManager.exe"
if exist "%BUILD_DIR%\Lab1_PassManager\Release\Lab1_PassManager.exe" set "PM_EXE=%BUILD_DIR%\Lab1_PassManager\Release\Lab1_PassManager.exe"

set "PR_EXE="
if exist "%BUILD_DIR%\Lab1_Protector\Lab1_Protector.exe" set "PR_EXE=%BUILD_DIR%\Lab1_Protector\Lab1_Protector.exe"
if exist "%BUILD_DIR%\Lab1_Protector\Release\Lab1_Protector.exe" set "PR_EXE=%BUILD_DIR%\Lab1_Protector\Release\Lab1_Protector.exe"

if not defined PM_EXE (
    echo [ОШИБКА] Lab1_PassManager.exe не найден в build/
    exit /b 1
)

copy /Y "!PM_EXE!" "%OUTPUT_DIR%\" >nul
if defined PR_EXE copy /Y "!PR_EXE!" "%OUTPUT_DIR%\" >nul
copy /Y "%~dp0Lab1_PassManager\credentials.json.enc" "%OUTPUT_DIR%\" >nul

"%QTDIR%\bin\windeployqt.exe" --release --no-translations --no-opengl-sw "%OUTPUT_DIR%\Lab1_PassManager.exe" >nul

if exist "!OPENSSL_DIR!\bin\libcrypto-3-x64.dll" (
    copy /Y "!OPENSSL_DIR!\bin\libcrypto-3-x64.dll" "%OUTPUT_DIR%\" >nul
    copy /Y "!OPENSSL_DIR!\bin\libssl-3-x64.dll" "%OUTPUT_DIR%\" >nul
    goto :deploy_done
)
if exist "!OPENSSL_DIR!\bin\libcrypto-3.dll" (
    copy /Y "!OPENSSL_DIR!\bin\libcrypto-3.dll" "%OUTPUT_DIR%\" >nul
    copy /Y "!OPENSSL_DIR!\bin\libssl-3.dll" "%OUTPUT_DIR%\" >nul
    goto :deploy_done
)
if exist "!OPENSSL_DIR!\libcrypto-3-x64.dll" (
    copy /Y "!OPENSSL_DIR!\libcrypto-3-x64.dll" "%OUTPUT_DIR%\" >nul
    copy /Y "!OPENSSL_DIR!\libssl-3-x64.dll" "%OUTPUT_DIR%\" >nul
)

:deploy_done
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
