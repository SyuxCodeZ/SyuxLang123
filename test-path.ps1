$syuxPath = "C:\Users\HP\OneDrive\Desktop\syux\SyuxLang"

# Add to current session PATH
$env:Path = "$syuxPath;$env:Path"

# Verify
Write-Host "Testing syux..."
& "$syuxPath\syux.exe" --version