# ufbx documentation

Documentation for [ufbx](https://github.com/bqqbarbhg/ufbx), a single source file FBX reader.

## Development

### Prerequisites

  - Install [cmake](https://cmake.org/) version >=3.10
    - apt-get has a recent enough version for Ubuntu 18.04 or Debian 10
  - Clone and install [emsdk](https://github.com/emscripten-core/emsdk)
  - Install [SHDC](https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md) from sokol-tools
    - [Prebuilt binaries](https://github.com/floooh/sokol-tools-bin)
    - [Source](https://github.com/floooh/sokol-tools)

### Getting started

```bash
# Build native/
cd native
cmake -B build -DCMAKE_TOOLCHAIN_FILE=/YOUR/INSTALL/PATH/emscripten/cmake/Modules/Platform/Emscripten.cmake
cmake --build build --config Release
cd ..

# Build script/
cd script
npm install
npm run build
cd ..

# Start hacking
npm install
npm run watch --serve

# Run simultaneously if you also want to hot-reload script/
# cd script && npm run watch
```
