@echo off
if "%~1"=="" (
  echo Usage: install-driver.bat path-to-passThrough.inf
  exit /b 2
)
pnputil /add-driver "%~1" /install
