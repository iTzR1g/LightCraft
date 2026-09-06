# MineLaunch

A super-minimalist Minecraft launcher built for low-end hardware.
Targets 32-bit Intel Atom with 1GB RAM running iceWM.

## Features

- Instance management (create, edit, delete Minecraft instances)
- Version selection (all official Mojang releases + snapshots)
- Memory/RAM configuration (conservative defaults for 1GB systems)
- Java auto-detection
- Automatic download of libraries, assets, and client jars
- Offline mode (no Microsoft account required)
- Vanilla/Forge/Fabric/Quilt/NeoForge loader selection
- FLTK-based GUI (~300KB, works great on iceWM)

## Build Dependencies

### Debian/Ubuntu
```sh
sudo apt install g++ libfltk1.3-dev libcurl4-openssl-dev pkg-config
```

### Arch Linux
```sh
sudo pacman -S base-devel fltk curl
```

### Void Linux (glibc or musl)
```sh
sudo xbps-install -S fltk-devel libcurl-devel gcc pkg-config
```

### Fedora
```sh
sudo dnf install gcc-c++ fltk-devel libcurl-devel
```

### Alpine
```sh
sudo apk add g++ fltk-dev curl-dev
```

## Build

```sh
make
```

For a 32-bit build on a 64-bit host (cross-compile):
```sh
make CXXFLAGS="-std=c++17 -Os -m32" LDFLAGS="-m32"
```

For a fully static binary (max portability):
```sh
make LDFLAGS="-static" LIBS="/usr/lib/libfltk.a /usr/lib/libcurl.a /usr/lib/libssl.a /usr/lib/libcrypto.a -lpthread -lz"
```

## Install

```sh
sudo make install
# or with custom prefix
sudo make install PREFIX=/usr
```

## Usage

```sh
minelaunch
# or with custom window size
minelaunch 600 400
```

Data is stored in `~/.minelaunch/`.

## Architecture

```
src/
  core/       - Platform utilities (filesystem, HTTP, JSON, Java, process)
  launcher/   - Minecraft logic (instances, downloads, version manifest, launch)
  ui/         - FLTK GUI
vendor/       - cJSON (vendored single-file JSON library)
```

## Design Goals

- **Tiny binary**: FLTK GUI is ~300KB, total binary under 1MB
- **Low RAM usage**: No unnecessary caching, streams downloads
- **Cross-distro**: No systemd dependency, works on musl/glibc/musl
- **32-bit support**: Compiled with `-m32` for Intel Atom targets
- **Zero config**: Auto-detects Java, sets conservative RAM defaults
