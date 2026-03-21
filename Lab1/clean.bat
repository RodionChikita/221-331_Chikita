@echo off
echo Очистка артефактов сборки...

cd /d "%~dp0Lab1_PassManager"
if exist Makefile nmake distclean >nul 2>&1
if exist release rmdir /s /q release
if exist debug rmdir /s /q debug
del /q *.stash 2>nul

cd /d "%~dp0Lab1_Protector"
if exist Makefile nmake distclean >nul 2>&1
if exist release rmdir /s /q release
if exist debug rmdir /s /q debug
del /q *.stash 2>nul

cd /d "%~dp0"
if exist dist rmdir /s /q dist

echo Готово.
