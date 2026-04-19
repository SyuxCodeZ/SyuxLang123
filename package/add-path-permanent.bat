@echo off
set SYUX_DIR=%~dp0
echo Adding Syux to PATH...

:: Get current user PATH
set "USER_PATH="

:: Add to user PATH using setx
setx PATH "%SYUX_DIR%;%PATH%" >nul 2>&1

echo.
echo Syux has been added to your PATH!
echo.
echo IMPORTANT: Close and reopen your terminal for changes to take effect.
echo.
pause