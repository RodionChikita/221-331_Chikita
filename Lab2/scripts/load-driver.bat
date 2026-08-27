@echo off
fltmc load PassThrough
if errorlevel 1 exit /b %errorlevel%
fltmc filters
