param(
  [string]$SourceFile = "example.syux"
)

$ErrorActionPreference = "Stop"

function Resolve-Gpp {
  if ($env:SYUX_GPP_PATH -and (Test-Path $env:SYUX_GPP_PATH)) {
    return $env:SYUX_GPP_PATH
  }

  $knownPaths = @(
    "D:\gcc msys2\ucrt64\bin\g++.exe",
    "C:\msys64\ucrt64\bin\g++.exe",
    "C:\msys64\mingw64\bin\g++.exe"
  )
  foreach ($path in $knownPaths) {
    if (Test-Path $path) {
      return $path
    }
  }

  $cmd = Get-Command g++ -ErrorAction SilentlyContinue
  if ($cmd) {
    return $cmd.Path
  }

  throw "g++ not found. Add it to PATH or set SYUX_GPP_PATH."
}

$workspace = Split-Path -Parent $PSScriptRoot
$sourcePath = Join-Path $workspace $SourceFile

if (!(Test-Path $sourcePath)) {
  throw "Source file not found: $sourcePath"
}

$tempRoot = "D:\Temp"
if (!(Test-Path $tempRoot)) {
  New-Item -ItemType Directory -Path $tempRoot | Out-Null
}

$gpp = Resolve-Gpp
Write-Host "Using g++: $gpp"
Write-Host "Using temp dir: $tempRoot"

$syuxExe = Join-Path $workspace "syux.exe"

Push-Location $workspace
try {
  $env:TEMP = $tempRoot
  $env:TMP = $tempRoot

  & $gpp -std=c++20 -I include src/main.cpp src/parser.cpp src/scanner.cpp src/codegen.cpp -o $syuxExe
  if ($LASTEXITCODE -ne 0) { throw "Failed to build syux compiler." }

  & $syuxExe run $sourcePath
  if ($LASTEXITCODE -ne 0) { throw "Syux run failed." }
}
finally {
  Pop-Location
}
