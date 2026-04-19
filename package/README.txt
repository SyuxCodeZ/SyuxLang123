# Syux - Portable Package

**Download, extract, done! No install needed.**

## Quick Start (30 seconds)

### 1. Add to PATH (one-time)
```batch
setup.bat
```

### 2. Code anywhere
Create `hello.syux`:
```syux
void.main [
    comp.out "Hello World!"
]
```

### 3. Run
```bash
syux hello.syux
```

That's it! No install, no config.

---

## Usage

```bash
syux hello.syux      # Run
syux build hello.syux  # Build only
syux transpile hello.syux  # Transpile to C++
```

---

## What's Included

| File | Purpose |
|------|---------|
| `syux.exe` | Compiler (works standalone!) |
| `setup.bat` | Add to PATH |
| `README.txt` | This file |

---

## VS Code (Optional)

For syntax highlighting:
1. Install extension: `Extensions → ... → Install from VSIX`
2. Select `syux-language.vsix`
3. VS Code will then recognize `.syux` and `.sx` files

But you can code in Notepad, vim, emacs, any editor!

---

## Why no VS Code auto-install?

VS Code extensions must be installed manually - that's how VS Code works. But the Syux compiler works anywhere with any editor.

---

## Troubleshooting

**"syux not found"**
Run `setup.bat` again or manually add this folder to PATH.

**VS Code doesn't recognize .syux files**
Reinstall the extension from `syux-language.vsix`

---

Syux is portable - move this folder anywhere, it works!