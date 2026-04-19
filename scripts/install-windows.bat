@echo off
setlocal EnableDelayedExpansion

set "INSTALL_DIR=%~dp0"
set "PATH_ENTRY=%INSTALL_DIR%"

echo ========================================
echo   Syux Compiler Setup
echo ========================================
echo.

:: Get user PATH
set "USER_PATH="

:: Check if already in PATH
echo Checking if Syux is already in PATH...
echo %PATH% | findstr /i /c:"syux" >nul
if !errorlevel! equ 0 (
    echo Syux is already in your PATH!
    echo.
    goto :done
)

:: Add to user PATH using setx (requires admin or user environment)
echo Adding Syux to PATH...

:: Try to add to current session PATH first
set "PATH=%PATH_ENTRY%;%PATH%"

:: Try to add to user-level PATH persistently
setx PATH "%PATH_ENTRY%;%PATH%" >nul 2>&1

if !errorlevel! equ 0 (
    echo Syux has been added to your PATH!
    echo.
    echo IMPORTANT: Please restart your terminal for changes to take effect.
    echo Or run: refreshenv (if using Git Bash or similar)
) else (
    echo Warning: Could not add to PATH persistently.
    echo Syux was added to your current session only.
)

:done
echo.
echo To verify installation, run: syux --version
echo.

:: Test if syux works in current session
if exist "%INSTALL_DIR%syux.exe" (
    echo Testing syux.exe...
    "%INSTALL_DIR%syux.exe" --version 2>nul || echo Note: --version not implemented yet
    echo.
    echo Installation complete!
) else (
    echo Error: syux.exe not found in %INSTALL_DIR%
    echo Please ensure syux.exe is in this folder.
)

pause