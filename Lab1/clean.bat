@echo off
echo Очистка артефактов сборки...

cd /d "%~dp0"
if exist build rmdir /s /q build
if exist dist rmdir /s /q dist

REM Удаление артефактов qmake (если были)
cd /d "%~dp0Lab1_PassManager"
if exist Makefile del /q Makefile* 2>nul
if exist release rmdir /s /q release
if exist debug rmdir /s /q debug
del /q *.stash 2>nul

cd /d "%~dp0Lab1_Protector"
if exist Makefile del /q Makefile* 2>nul
if exist release rmdir /s /q release
if exist debug rmdir /s /q debug
del /q *.stash 2>nul

echo Готово.
