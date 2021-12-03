.\build\sokol-shdc.exe -l glsl300es -i .\wasm\shader\copy.glsl -o .\wasm\shader\copy.h
em++ -Wno-deprecated -s "EXTRA_EXPORTED_RUNTIME_METHODS=[GL]" -s RESERVED_FUNCTION_POINTERS=2 -s USE_WEBGL2=1 -s ENVIRONMENT=web .\wasm\viewer.cpp .\wasm\ufbx.c .\wasm\json_output.cpp .\wasm\external.c -o build\viewer.js
