@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\Prototype.ps1" -Action Play %*
if errorlevel 1 pause
