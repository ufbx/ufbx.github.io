
if (typeof window.Module === "undefined") {
    window.Module = {}
}

const Module = window.Module

export function rpcOnReady(cb)
{
    if (Module.hasRun) {
        cb()
    } else {
        Module.onRuntimeInitialized = cb
    }
}

export function rpcSetup()
{
    Module._js_setup()
}

export function rpcReload()
{
    Module._js_setup()
}

export function rpcDestroy()
{
    Module._js_destroy_context()
}

export function rpcCall(input)
{
    // console.log(">", input)
    const inStr = JSON.stringify(input)
    const inSize = inStr.length * 4 + 1
    const inPtr = Module._malloc(inSize)
    stringToUTF8(inStr, inPtr, inSize)

    const outPtr = Module._js_rpc(inPtr)

    const outStr = UTF8ToString(outPtr)
    Module._free(outPtr)
    const output = JSON.parse(outStr)
    // console.log("<", output)

    if (output.error) {
        console.error(output.error)
    }

    return output
}

export function rpcMemory()
{
    return HEAPU8.buffer
}

export function rpcHeapU8()
{
    return HEAPU8
}

export function rpcGl()
{
    return GLctx
}

export function rpcLoadScene(name, buffer) {
    const size = buffer.byteLength
    const dataPointer = Module._malloc(size)
    if (!dataPointer) throw new Error("Out of memory!")
    HEAPU8.set(new Uint8Array(buffer), dataPointer)
    const scene = rpcCall({ cmd: "loadScene", name, dataPointer, size })
    Module._free(dataPointer)
    return scene.scene
}
