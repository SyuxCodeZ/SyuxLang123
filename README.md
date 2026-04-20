# Syux

**Syux** is a statically-typed programming language that transpiles to C++, designed to sit between Python and C++ — offering Python's simplicity with C++'s power and structure.

> "No manual memory management pain, but still powerful."

---

## What is Syux?

Syux is a modern programming language built for developers who want:
- **Structure** — static typing with type inference
- **Simplicity** — clean, readable syntax
- **Power** — native performance via C++ transpilation
- **Safety** — RAII-based resource management

**Who is it for?**
- Beginners learning programming concepts
- Educators teaching systems programming
- Developers who want C++ power without the complexity

---

## Quick Example

```syux
class User [
  val name = "Guest"

  func greet(msg) [
    comp.out msg + ", " + self.name + "!"
  ]
]

void.main [
  val u = User("Sid")
  u.greet("Hello")
  set u.name = "Alex"
  comp.out u.name
]
```

---

## Features (v0.7.1)

| Feature | Description |
|---------|-------------|
| Variables | `val` (immutable) / `set` (mutable) |
| Types | `int`, `float`, `string`, `bool` (`true/false`, `on/off`) |
| Arrays | With `.len`, `.first`, `.last`, `.sum`, `.max`, `.min`, `.avg`, `.sort`, `.join`, `.includes` |
| Array Methods | `.map`, `.filter`, `.reduce` |
| Loops | `for`, `while`, for-each |
| Functions | First-class functions, lambdas |
| Classes | Constructors (`ctor`), Destructors (`dtor`), **Methods** (`func`) |
| Method Calls | `object.method(args)` |
| Property Access | `object.property` |
| Property Assignment | `set object.property = value` |
| Increment/Decrement | `i++`, `i--` |
| Transpilation | Generates C++20 code |
| Standard Library | `add library stl`, `math`, `rand`, `time`, `file`, `json`, `http`, `regex`, `game`, `gui`, `data`, `crypto`, `sys`, `thread` |

---

## Getting Started

### Quick Install (Windows)

1. **Run the setup script:**
   ```powershell
   .\scripts\setup.ps1
   ```
   
2. **Restart your terminal**

3. **Run:**
   ```bash
   syux --version
   syux example.syux
   ```

### Manual Build

```bash
g++ -std=c++20 -I include -o syux src/main.cpp src/scanner.cpp src/parser.cpp src/codegen.cpp
```

### Run

```bash
syux program.syux      # Run (transpile + compile + run)
syux run program.syux  # Same as above
syux build program.syux  # Build (transpile + compile, don't run)
syux transpile program.syux  # Transpile to C++ only
```

---

## Standard Library

Use `add library <name>` to include library features:

| Library | Functions |
|---------|-----------|
| `stl` | Array methods: `.len`, `.first`, `.last`, `.sum`, `.max`, `.min`, `.avg`, `.map`, `.filter`, `.reduce`, `.sort`, `.join`, `.includes` |
| `math` | `sqrt`, `abs`, `round`, `pow`, `floor`, `ceil`, `sin`, `cos`, `tan`, `log`, `exp`, etc. |
| `rand` | `random`, `randInt`, `shuffle` |
| `time` | `now`, `sleep`, `timestamp` |
| `file` | `read`, `write`, `append`, `exists` |
| `json` | `parse`, `stringify` |
| `http` | `get`, `post` |
| `regex` | `match`, `replace` |
| `game` | Game library (Windows) |
| `gui` | GUI library (Windows/X11) |
| `data` | `fromCSV`, `median`, `variance`, `stdDev`, `mode` |
| `crypto` | `md5`, `sha256`, `base64Encode`, `xorEncrypt` |
| `sys` | `exec`, `getEnv`, `setEnv`, `osInfo`, `cpuCores`, `memAvailable`, `clipboard` |
| `thread` | `run`, `sleep`, `atomicGet`, `atomicSet` |

### Using Libraries

```syux
add library crypto
add library math

void.main [
    val hash = syxcrypto.md5("hello")
    val x = math.sqrt(16)
    comp.out hash
    comp.out x
]
```

---

## Installation

## Roadmap

| Version | Goal |
|---------|------|
| 0.4.0 | Basic features, classes (ctor/dtor) |
| 0.5.0 | OOP improvements, methods, property access |
| 0.6.0 | Standard Library, Arrays STL methods, crypto, data, sys, thread |
| **0.7.0** | **Version bump for release** |
| 1.0.0 | Stable release |

---

## Why Syux?

| | Python | Syux | C++ |
|---|--------|------|-----|
| **Types** | Dynamic | Static + inference | Static |
| **Complexity** | Simple | Simple + structured | Complex |
| **Performance** | Slow | Fast (C++) | Fast |
| **Memory** | Automatic | RAII | Manual |

---

*Created by a student passionate about making programming accessible.*