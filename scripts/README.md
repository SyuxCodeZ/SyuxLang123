# Syux Installation Scripts

This folder contains installation scripts for adding Syux to your system PATH and setting up a C++ compiler.

## Quick Install

### Windows
Simply double-click `install-windows.bat` - it will:
1. Check for a C++ compiler (or prompt you to install one)
2. Add Syux to your PATH
3. Create a launcher script

Or use `install-windows-full.bat` for a more interactive setup.

### Linux
```bash
chmod +x install-linux.sh
./install-linux.sh
```

### macOS
```bash
chmod +x install-macos.sh
./install-macos.sh
```

## Manual Setup

If you prefer to set up manually:

1. **Ensure you have a C++ compiler:**
   - Windows: Install [TDM-GCC](https://sourceforge.net/projects/tdm-gcc/) or [MSYS2](https://www.msys2.org/)
   - Linux: `sudo apt install g++` (Ubuntu) or equivalent
   - macOS: Install Xcode Command Line Tools

2. **Add Syux to PATH:**
   - Windows: Add the Syux folder to your system PATH
   - Linux/macOS: Add `export PATH="/path/to/syux:$PATH"` to your `.bashrc` or `.zshrc`

3. **Compile the compiler:**
   ```bash
   g++ -std=c++20 -I include -o syux src/main.cpp src/scanner.cpp src/parser.cpp src/codegen.cpp
   ```

## Usage

Once installed:
```bash
syux file.syux        # Run (transpile + compile + run)
syux run file.syux    # Same as above
syux build file.syux  # Build (transpile + compile, don't run)
syux transpile file.syux  # Transpile to C++ only
```

## Troubleshooting

### "Command not found" on Windows
- Run the install script again
- Or manually add the Syux folder to your PATH

### No compiler found
- Install a C++ compiler (see above)
- Make sure it's in your PATH

### Permission denied (Linux/macOS)
- Make scripts executable: `chmod +x install-*.sh`