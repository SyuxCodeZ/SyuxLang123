@echo off
setlocal enabledelayedexpansion

echo ================================================
echo   Syux v0.5.0 Setup
echo ================================================
echo.

set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

echo [1/4] Scanning for C++ compilers...

set "FOUND_CLANG="
set "FOUND_GCC="

REM Check common installation paths
set "paths=D:\clang\bin D:\llvm\bin C:\LLVM\bin D:\gcc msys2\ucrt64\bin C:\mingw64\bin C:\msys64\ucrt64\bin %PROGRAMFILES%\LLVM\bin %PROGRAMFILES%\CodeBlocks\MinGW\bin"

for %%p in (%paths%) do (
    if exist "%%p\clang++.exe" (
        if not defined FOUND_CLANG set "FOUND_CLANG=%%p"
    )
    if exist "%%p\g++.exe" (
        if not defined FOUND_GCC set "FOUND_GCC=%%p"
    )
)

REM Try to find in PATH
where clang++ >nul 2>&1
if !errorlevel! equ 0 (
    if not defined FOUND_CLANG (
        for /f "delims=" %%i in ('where clang++') do (
            set "FOUND_CLANG=%%~dpi"
        )
    )
)

where g++ >nul 2>&1
if !errorlevel! equ 0 (
    if not defined FOUND_GCC (
        for /f "delims=" %%i in ('where g++') do (
            set "FOUND_GCC=%%~dpi"
        )
    )
)

if defined FOUND_CLANG echo   Found Clang++: !FOUND_CLANG!
if defined FOUND_GCC echo   Found GCC: !FOUND_GCC!
if not defined FOUND_CLANG if not defined FOUND_GCC echo   WARNING: No compiler found!

echo.
echo [2/4] Adding Syux to PATH...

set "USER_PATH="
reg query "HKCU\Environment" /v Path >nul 2>&1
if !errorlevel! equ 0 (
    for /f "skip=2 tokens=*" %%a in ('reg query "HKCU\Environment" /v Path') do set "USER_PATH=%%a"
)

echo !USER_PATH! | find /i "%SCRIPT_DIR%" >nul 2>&1
if !errorlevel! equ 0 (
    echo   Syux already in PATH
) else (
    set "NEW_PATH=!USER_PATH;!SCRIPT_DIR!"
    reg add "HKCU\Environment" /v Path /t REG_SZ /d "!NEW_PATH!" /f >nul 2>&1
    echo   Added Syux to PATH
)

echo.
echo [3/4] Adding C++ compiler to PATH...

if defined FOUND_CLANG (
    echo !USER_PATH! | find /i "!FOUND_CLANG!" >nul 2>&1
    if !errorlevel! equ 0 (
        echo   Clang++ already in PATH
    ) else (
        set "NEW_PATH=!USER_PATH;!FOUND_CLANG!"
        reg add "HKCU\Environment" /v Path /t REG_SZ /d "!NEW_PATH!" /f >nul 2>&1
        echo   Added Clang++ to PATH
    )
    goto :add_done
)

if defined FOUND_GCC (
    echo !USER_PATH! | find /i "!FOUND_GCC!" >nul 2>&1
    if !errorlevel! equ 0 (
        echo   GCC already in PATH
    ) else (
        set "NEW_PATH=!USER_PATH;!FOUND_GCC!"
        reg add "HKCU\Environment" /v Path /t REG_SZ /d "!NEW_PATH!" /f >nul 2>&1
        echo   Added GCC to PATH
    )
)

:add_done

echo.
echo [4/4] Broadcasting PATH change...

set "USERENVIRONMENT=CHANGED"

echo.
echo ================================================
echo   Setup Complete!
echo ================================================
echo.
echo Please CLOSE and REOPEN your terminal to use syux.
echo.
echo Usage:
echo   syux file.syux        - Run
echo   syux build file.syux    - Build
echo   syux transpile file.syux - Transpile
echo.
pause
endlocal