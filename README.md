# Halftone
2D Animation in a 3D way

Halftone is a production-focused 2D/2.5D animation suite. It combines
hand-drawn character animation with stylised, hand-painted 3D environments.

Splash → **Project Browser** (projects, subprojects and scenes, each with
scoped asset packs, build systems, macros and configs) → **Scene Editor**,
which opens on an empty 3D view. Add canvases, models and lights, then work
on any element: a canvas opens in a 2D animation view (layers, blend modes,
brushes) or a 3D view of its layers, and a model opens in a material and
texture-paint view.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the module layout, the
file formats, and where each feature plugs in.

## Requirements

* Clang with C++23 modules support: clang 18 or newer, or clang 20+ for the
  Windows target (`--target=x86_64-w64-mingw32`, llvm-mingw / MSYS2 clang64).
* Python 3 and [SCons](https://scons.org) (`pip install scons`)
* A Vulkan 1.3 driver. The Vulkan SDK is optional and only adds validation
  layers.
* SDL3. Windows binaries are vendored in `build/modules/SDL3.module/bin`. On
  Linux or macOS, set `HALFTONE_SDL3_DIR` to an SDL3 install prefix, or
  install it system-wide.

## Build

```sh
scons                    # debug build  -> build/bin/Halftone(.exe)
scons variant=release    # optimised
scons run                # build + launch
scons gen_lspinfo        # refresh compile_commands.json for clangd
scons -c                 # clean
scons --cxx=path/to/clang++
```

`python build/do_build_modules` does the same build without SCons.

Set `HALFTONE_MAX_FRAMES=N` to make the app quit after N frames. This is
useful for smoke tests.

Projects live in `<user prefs dir>/Projects` by default
(`%APPDATA%\CitronLabs\Halftone\Projects` on Windows). Set `/Projects/Library`
in `Preferences.json` to use a shared studio folder instead.

## Layout

```
lib/Halftone.<Module>/      module interfaces (.cppm)
src/Lib/<Module>/           module implementations (.cpp)
src/App/                    entry point + App module implementation
lib/__Vendor, src/Lib/__Vendor   third-party headers / sources
build/config.py             build manifest: modules, flags, platforms
build/modules.py            module scanner + command generation
build/modules/<Lib>.module/bin   prebuilt third-party binaries
docs/                       architecture notes
```
