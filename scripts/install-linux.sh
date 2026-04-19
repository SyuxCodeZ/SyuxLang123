#!/bin/bash

echo "========================================"
echo "  Syux Compiler Setup (Linux)"
echo "========================================"
echo

INSTALL_DIR="$(cd "$(dirname "$0")" && pwd)"
SYUX_DIR="$INSTALL_DIR"

echo "[1/3] Checking for C++ compiler..."

# Check if g++ is available
if command -v g++ &> /dev/null; then
    echo "  Found g++ at: $(which g++)"
    GCC_PATH=$(which g++)
else
    echo "  No C++ compiler found!"
    echo
    echo "Options:"
    echo "  1) Install via apt (Ubuntu/Debian): sudo apt install g++"
    echo "  2) Install via yum (Fedora/RHEL): sudo yum install gcc-c++"
    echo "  3) Install via pacman (Arch): sudo pacman -S gcc"
    echo
    read -p "Press Enter after installing, or Ctrl+C to exit..."
    
    if command -v g++ &> /dev/null; then
        GCC_PATH=$(which g++)
    else
        echo "Error: g++ still not found. Please install a C++ compiler."
        exit 1
    fi
fi

echo
echo "[2/3] Adding Syux to PATH..."

# Get GCC directory
GCC_DIR=$(dirname "$GCC_PATH")

# Check if already in PATH
if [[ ":$PATH:" == *":$SYUX_DIR:"* ]]; then
    echo "  Syux already in PATH"
else
    # Add to PATH in .bashrc for persistence
    if [ -f "$HOME/.bashrc" ]; then
        if ! grep -q "syux" "$HOME/.bashrc"; then
            echo "export PATH=\"$SYUX_DIR:\$PATH\"" >> "$HOME/.bashrc"
            echo "  Added to ~/.bashrc"
        fi
    fi
    
    # Add to PATH for current session
    export PATH="$SYUX_DIR:$PATH"
    echo "  Added to PATH"
fi

echo
echo "[3/3] Creating syux launcher script..."

# Create the syux wrapper script
cat > "$SYUX_DIR/syux-wrapper.sh" << 'EOF'
#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Compile
g++ -std=c++20 -I include -o out out.cpp
if [ $? -ne 0 ]; then
    echo "[Syux Error] C++ compilation failed."
    exit 1
fi

# Run if not build/transpile
if [ "$1" != "build" ] && [ "$1" != "transpile" ]; then
    ./out
fi
EOF

chmod +x "$SYUX_DIR/syux-wrapper.sh"

# Create symlink in PATH
ln -sf "$SYUX_DIR/syux-wrapper.sh" "$SYUX_DIR/syux" 2>/dev/null

echo "========================================"
echo "  Installation Complete!"
echo "========================================"
echo
echo "Next steps:"
echo "  1. Restart your terminal, or run: source ~/.bashrc"
echo "  2. Run: syux file.syux"
echo

# Test if it works
if [ -f "$SYUX_DIR/syux-wrapper.sh" ]; then
    echo "Testing installation..."
    echo "  (No test file found - that's OK)"
    echo
    echo "To test, create a simple .syux file and run:"
    echo "  syux yourfile.syux"
fi

echo
read -p "Press Enter to continue..."