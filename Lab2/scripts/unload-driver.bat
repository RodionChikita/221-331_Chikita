@echo off
fltmc unload PassThrough
if errorlevel 1 exit /b %errorlevel%
fltmc filters
