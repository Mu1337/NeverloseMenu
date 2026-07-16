@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\..\run_menu.ps1"
if errorlevel 1 pause
