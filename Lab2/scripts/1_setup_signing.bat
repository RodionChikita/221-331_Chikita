@echo off
cd /d "%~dp0"
echo [1/3] Импорт тестового сертификата в доверенные корневые и издателей...
certutil -addstore -f Root "%~dp0Lab2TestCert.cer"
certutil -addstore -f TrustedPublisher "%~dp0Lab2TestCert.cer"
echo.
echo [2/3] Включение тестового режима подписи драйверов (testsigning)...
bcdedit /set testsigning on
echo.
echo [3/3] Требуется перезагрузка, чтобы testsigning вступил в силу.
echo После перезагрузки запустите 2_load_and_demo.bat (тоже от администратора).
echo.
pause
shutdown /r /t 5 /c "Lab2: enabling test signing"
