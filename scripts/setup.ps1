# Syux Installer
# Run this script as Administrator on Windows

param(
    [switch]$Uninstall
)

$ErrorActionPreference = "Stop"

$INSTALL_DIR = Split-Path -Parent $PSScriptRoot
$SYUX_EXE = Join-Path $INSTALL_DIR "syux.exe"

function Write-Step {
    param([string]$Message)
    Write-Host "`n[Syux] $Message" -ForegroundColor Cyan
}

function Add-ToPath {
    param([string]$PathToAdd)
    
    $currentPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if ($currentPath -notlike "*$PathToAdd*") {
        $newPath = "$PathToAdd;$currentPath"
        [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
        $env:Path = "$PathToAdd;$env:Path"
        return $true
    }
    return $false
}

function Find-Compiler {
    # Check common GCC locations on Windows
    $paths = @(
        "D:\gcc msys2\ucrt64\bin\g++.exe",
        "C:\msys64\ucrt64\bin\g++.exe",
        "C:\msys64\mingw64\bin\g++.exe",
        "C:\mingw64\bin\g++.exe",
        "C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v5-rev0\mingw64\bin\g++.exe"
    )
    
    foreach ($p in $paths) {
        if (Test-Path $p) {
            return $p
        }
    }
    
    # Check if g++ is in PATH
    $cmd = Get-Command g++ -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }
    
    return $null
}

function Test-SyuxInstalled {
    $syuxInPath = Get-Command syux -ErrorAction SilentlyContinue
    if ($syuxInPath) {
        return $true
    }
    return $false
}

# ============================================
# Main Script
# ============================================

if ($Uninstall) {
    Write-Step "Uninstalling Syux..."
    $currentPath = [Environment]::GetEnvironmentVariable("Path", "User")
    $newPath = ($currentPath -split ';' | Where-Object { $_ -notlike "*SyuxLang*" }) -join ';'
    [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
    Write-Host "Syux removed from PATH." -ForegroundColor Green
    exit 0
}

Write-Host @"

========================================
   Syux Compiler v0.5.0 - Setup
========================================

"@ -ForegroundColor Yellow

# Check if syux.exe exists
if (!(Test-Path $SYUX_EXE)) {
    Write-Host "Error: syux.exe not found in $INSTALL_DIR" -ForegroundColor Red
    Write-Host "Please compile the compiler first:" -ForegroundColor Yellow
    Write-Host "  g++ -std=c++20 -I include -o syux.exe src/main.cpp src/scanner.cpp src/parser.cpp src/codegen.cpp"
    exit 1
}

Write-Step "Checking C++ compiler..."
$compiler = Find-Compiler
if ($compiler) {
    Write-Host "  Found: $compiler" -ForegroundColor Green
} else {
    Write-Host @"

  No C++ compiler found!
  
  You need to install GCC to compile the C++ output from Syux.
  
  Recommended: MSYS2
  1. Download from: https://www.msys2.org/
  2. Install MSYS2
  3. Run: pacman -S mingw-w64-ucrt-x86_64-gcc
  4. Add to PATH: C:\msys64\ucrt64\bin
  
  After installing, run this setup again.
"@ -ForegroundColor Yellow
    $installNow = Read-Host "Open MSYS2 download page? (y/n)"
    if ($installNow -eq 'y') {
        Start-Process "https://www.msys2.org/"
    }
    exit 1
}

Write-Step "Adding Syux to PATH..."
$added = Add-ToPath $INSTALL_DIR
if ($added) {
    Write-Host "  Added to PATH: $INSTALL_DIR" -ForegroundColor Green
} else {
    Write-Host "  Already in PATH" -ForegroundColor Yellow
}

Write-Host @"

========================================
   Setup Complete!
========================================

Next steps:
1. Restart your terminal (or open a new one)
2. Run: syux --version

Usage:
  syux file.syux        Run file (transpile + compile + run)
  syux run file.syux   Same as above
  syux build file.syux Build only (transpile + compile)
  syux transpile file.syux  Transpile to C++ only

"@ -ForegroundColor Green

# Test installation
Write-Step "Testing installation..."
$env:Path = [Environment]::GetEnvironmentVariable("Path", "User")
try {
    $result = & syux.exe --version 2>&1
    Write-Host "  $result" -ForegroundColor Green
} catch {
    Write-Host "  Could not run syux (restart terminal and try again)" -ForegroundColor Yellow
}

pause