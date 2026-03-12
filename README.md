# 🌙 MoonLanding

A small retro game built with the **Allegro 4** library, compiled on NixOS.

## 🚀 Getting Started

A Nix flake is included to ensure a reproducible development environment with Allegro and all required dependencies.

## 🛠️ Compiling

Enter the development environment:
```bash
nix develop
```

Then compile with:
```bash
g++ moonlanding.cpp $(pkg-config --cflags --libs allegro) -lm -o moonlanding
```

## ▶️ Running

A pre-compiled binary is already provided. Just run:
```bash
./moonlanding
```
## 📸 Screenshot

![MoonLanding gameplay](assets/screenshot.png)
