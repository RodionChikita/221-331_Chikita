@echo off
cd /d "%~dp0"
set F=secret.lab2ext

echo === [1] Установка драйвера ===
echo Способ A (может не зарегистрировать службу у некоторых версий Windows):
echo   ПКМ по passThrough.inf -> "Установить"
echo.
echo Способ B (надёжный, используется ниже автоматически) —
echo   регистрация службы фильтра напрямую через sc.exe + реестр Instances.
copy /y passThrough.sys "%SystemRoot%\System32\drivers\PassThrough.sys" >nul
sc.exe query PassThrough >nul 2>&1
if errorlevel 1 (
    sc.exe create PassThrough type= filesys start= demand error= normal binPath= "%SystemRoot%\System32\drivers\PassThrough.sys" DisplayName= "Lab2 PassThrough" >nul
    sc.exe config PassThrough depend= FltMgr group= "FSFilter Encryption" >nul
    reg add "HKLM\SYSTEM\CurrentControlSet\Services\PassThrough\Instances" /v DefaultInstance /t REG_SZ /d "PassThrough Instance" /f >nul
    reg add "HKLM\SYSTEM\CurrentControlSet\Services\PassThrough\Instances\PassThrough Instance" /v Altitude /t REG_SZ /d "147000" /f >nul
    reg add "HKLM\SYSTEM\CurrentControlSet\Services\PassThrough\Instances\PassThrough Instance" /v Flags /t REG_DWORD /d 0 /f >nul
    echo Служба PassThrough зарегистрирована.
) else (
    echo Служба PassThrough уже зарегистрирована.
)
echo.
echo === [2] Загрузка драйвера ===
fltmc load PassThrough
fltmc
echo.

echo === [3] ДЕМО: файл ДО шифрования на диске ===
test_file_io.exe write %F% "Hello, Lab2 transparent encryption!"
echo (драйвер загружен; перезапишем, чтобы на диске оказался шифртекст)
test_file_io.exe write %F% "Hello, Lab2 transparent encryption!"
echo.
echo --- Чтение нашим приложением (драйвер расшифровывает -> читаемо): ---
test_file_io.exe read %F%
echo.
echo --- Теперь откройте %F% в notepad.exe: содержимое НЕчитаемо (шифртекст на диске) ---
echo    notepad %F%
pause

echo === [4] Выгрузка драйвера и чтение снова (докажем, что на диске шифртекст) ===
fltmc unload PassThrough
echo --- Чтение БЕЗ драйвера (сырой шифртекст -> нечитаемо): ---
test_file_io.exe read %F%
echo.
echo Готово. Для просмотра отладочной печати драйвера используйте DebugView
echo (Capture Kernel) внутри этой ВМ.
pause
