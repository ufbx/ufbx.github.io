# ufbx documentation

Documentation for [ufbx](https://github.com/bqqbarbhg/ufbx), a single source file FBX reader.

## Development

### Prerequisites

  - Clone and install [emsdk](https://github.com/emscripten-core/emsdk)
  - Install sokol-tools
    - [Prebuild binaries](https://github.com/floooh/sokol-tools-bin)
    - [Source](https://github.com/floooh/sokol-tools)

### Linux

```
# Install Node.js modules
npm install

# Build native/
cd native
cmake -B build -DCMAKE_TOOLCHAIN_FILE=/YOUR/INSTALL/PATH/emscripten/cmake/Modules/Platform/Emscripten.cmake
cmake --build build -j 24
cd ..

# Build script/
cd script
npm run build
cd ..

# Start hacking
npm run watch --serve
```
