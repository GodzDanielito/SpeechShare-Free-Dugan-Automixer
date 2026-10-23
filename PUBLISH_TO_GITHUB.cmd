@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0PUBLISH_TO_GITHUB.ps1"
echo.
pause
