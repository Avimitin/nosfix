@echo off
setlocal
title nosfix configuration

set "SCRIPT=%~dp0nosfix-config-gui.ps1"
if not exist "%SCRIPT%" (
    echo Cannot find "%SCRIPT%".
    echo Keep this .bat next to nosfix-config-gui.ps1.
    pause
    exit /b 1
)

where powershell.exe >nul 2>nul
if errorlevel 1 (
    echo Windows PowerShell was not found in PATH.
    pause
    exit /b 1
)

cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT%" %*
set "RESULT=%ERRORLEVEL%"

if not "%RESULT%"=="0" (
    echo.
    echo nosfix configuration exited with code %RESULT%.
    pause
)

endlocal & exit /b %RESULT%
