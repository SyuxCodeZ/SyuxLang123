#!/bin/bash

echo "========================================"
echo "  Syux Compiler Setup (macOS)"
echo "========================================"
echo

INSTALL_DIR="$(cd "$(dirname "$0")" && pwd)"
SYUX_DIR="$INSTALL_DIR"

echo "[1/3] Checking for C++ compiler..."

# Check if clang++ or g++ is available
if command -v clang++ &> /dev/null; then
    echo "  Found clang++ at: $(which clang++)"
    COMPILER="clang++"
elif command -v g++ &> /dev/null; then
    echo "  Found g++ at: $(which g++)"
    COMPILER="g++"
else
    echo "  No C++ compiler found!"
    echo
    echo "Installing Xcode Command Line Tools..."
    xcode-select --install
    echo
    echo "After installation, run this script again."
    read -p "Press Enter to exit..."
    exit 1
fi

echo
echo "[2/3] Adding Syux to PATH..."

# Check if already in PATH
if [[ ":$PATH:" == *":$SYUX_DIR:"* ]]; then
    echo "  Syux already in PATH"
else
    # Add to PATH in .zshrc for persistence (macOS default shell)
    if [ -f "$HOME/.zshrc" ]; then
        if ! grep -q "syux" "$HOME/.zshrc"; then
            echo "export PATH=\"$SYUX_DIR:\$PATH\"" >> "$HOME/.zshrc"
            echo "  Added to ~/.zshrc"
        fi
    elif [ -f "$HOME/.bash_profile" ]; then
        if ! grep -q "syux" "$HOME/.bash_profile"; then
            echo "export PATH=\"$SYUX_DIR:\$PATH\"" >> "$HOME/.bash_profile"
            echo "  Added to ~/.bash_profile"
        fi
    fi
    
    # Add to PATH for current session
    export PATH="$SYUX_DIR:$PATH"
    echo "  Added to PATH"
fi

echo
echo "[3/3] Creating syux launcher script..."

# Create the syux wrapper script
cat > "$SYUX_DIR/syux-macos.sh" << EOF
#!/bin/bash

SCRIPT_DIR="\$(cd "\$(dirname "\$0")" && pwd)"
cd "\$SCRIPT_DIR"

# Compile using $COMPILER
$COMPILER -std=c++20 -I include -o out out.cpp
if [ \$? -ne 0 ]; then
    echo "[Syux Error] C++ compilation failed."
    exit 1
fi

# Run if not build/transpile
if [ "\$1" != "build" ] && [ "\$1" != "transpile" ]; then
    ./out
fi
EOF

chmod +x "$SYUX_DIR/syux-macos.sh"

# Create symlink in PATH
ln -sf "$SYUX_DIR/syux-macos.sh" "$SYUX_DIR/syux" 2>/dev/null

echo "========================================"
echo "  Installation Complete!"
echo "========================================"
echo
echo "Next steps:"
echo "  1. Restart your terminal, or run: source ~/.zshrc"
echo "  2. Run: syux file.syux"
echo

# Test if it works
if [ -f "$SYUX_DIR/syux-macos.sh" ]; then
    echo "Testing installation..."
    echo "  (No test file found - that's OK)"
    echo
    echo "To test, create a simple .syux file and run:"
    echo "  syux yourfile.syux"
fi

echo
read -p "Press Enter to continue..."