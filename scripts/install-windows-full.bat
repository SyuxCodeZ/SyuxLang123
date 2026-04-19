@echo off
setlocal EnableDelayedExpansion

set "INSTALL_DIR=%~dp0"
set "SYUX_DIR=%~dp0"

echo ========================================
echo   Syux Compiler Setup
echo ========================================
echo.

:: ========================================
:: Step 1: Find or install GCC
:: ========================================
echo [1/3] Checking for C++ compiler...

:: Check if g++ is in PATH
where g++ >nul 2>&1
if %errorlevel% equ 0 (
    echo   Found g++ in PATH
    for /f "delims=" %%i in ('where gpx') do set "GCC_PATH=%%i"
    goto :compiler_found
)

:: Check common installation paths
set "GCC_PATHS=C:\mingw64\bin\g++.exe C:\msys64\mingw64\bin\g++.exe C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v5-rev0\mingw64\bin\g++.exe"
for %%p in (%GCC_PATHS%) do (
    if exist "%%p" (
        set "GCC_PATH=%%p"
        goto :compiler_found
    )
)

:: No compiler found - offer to download
echo   No C++ compiler found!
echo.
echo Options:
echo   1) Download and install TDM-GCC (recommended, ~100MB)
echo   2) I already have GCC installed somewhere else
echo   3) Skip (use existing system g++ if available)
echo.

set /p choice="Enter your choice (1-3): "

if "%choice%"=="1" (
    echo Downloading TDM-GCC...
    powershell -Command "Invoke-WebRequest -Uri 'https://sourceforge.net/projects/tdm-gcc/files/TDM-GCC%20Installer/tdm64-gcc-10.3.0.exe/download' -OutFile '%TEMP%\tdm-gcc-installer.exe'"
    echo.
    echo Please complete the installation manually.
    echo Then run this setup again.
    pause
    exit /b 1
) else if "%choice%"=="2" (
    echo Please enter the path to your g++.exe
    set /p GCC_PATH="g++ path: "
    if not exist "%GCC_PATH%" (
        echo Error: File not found!
        pause
        exit /b 1
    )
) else (
    echo Using system default (may not work)
)

:compiler_found
echo   Using compiler: %GCC_PATH%

:: ========================================
:: Step 2: Add Syux to PATH
:: ========================================
echo.
echo [2/3] Adding Syux to PATH...

:: Check if already in PATH
echo %PATH% | findstr /i /c:"syux" >nul
if %errorlevel% equ 0 (
    echo   Syux already in PATH
) else (
    :: Add to PATH for current session
    set "PATH=%SYUX_DIR%;%PATH%"
    
    :: Try to add to user PATH persistently
    setx PATH "%SYUX_DIR%;%PATH%" >nul 2>&1
    
    if %errorlevel% equ 0 (
        echo   Added to PATH
    ) else (
        echo   Warning: Could not add to PATH persistently
    )
)

:: ========================================
:: Step 3: Create launcher script
:: ========================================
echo.
echo [3/3] Creating syux launcher...

:: Create a batch file that sets up the environment
(
echo @echo off
echo set "SYUX_DIR=%SYUX_DIR:\=\\%%"
echo set "GCC_DIR=%GCC_PATH:\=\\%"
echo set "GCC_DIR=!GCC_DIR:~0,-9!"
echo "%%GCC_DIR%%\g++.exe" -std=c++20 -I "%%SYUX_DIR%%include" -o out.exe out.cpp
echo if %%errorlevel%% equ 0 if "%%1" neq "build" if "%%1" neq "transpile" ^(
echo     .\\out.exe
echo ^)
) > "%SYUX_DIR%syux.bat"

echo   Created syux.bat

:: ========================================
:: Summary
:: ========================================
echo.
echo ========================================
echo   Installation Complete!
echo ========================================
echo.
echo Next steps:
echo   1. Restart your terminal
echo   2. Run: syux file.syux
echo.
echo To test now without restart, run:
echo   set PATH=%SYUX_DIR%;%%PATH%%
echo.

pause