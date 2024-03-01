
let worker = null
let callbackToken = 0
let callbacks = new Map()

function rpcSend(message, extra=undefined) {
    return new Promise((resolve, reject) => {
        let token = ++callbackToken
        // console.log(token, "::", message)
        callbacks.set(token, resolve)
        worker.postMessage({
            message, token,
            type: "setup",
        }, extra)
    })
}

export function rpcOnReady(cb)
{
    worker = new Worker("/native/js_viewer.js")
    worker.addEventListener("message", (e) => {
        const { token, result } = e.data
        let cb = callbacks.get(token)
        // console.log(token, "->", result)
        cb(result)
        callbacks.delete(token)
    })
    rpcSend({
        type: "init",
    }).then(() => {
        // console.log("RPC ready")
        cb()
    })
}

export async function rpcSetup()
{
    const offscreenCanvas = document.querySelector("#ufbx-render-canvas").transferControlToOffscreen()
    await rpcSend({
        type: "setup",
        canvas: offscreenCanvas,
    }, [offscreenCanvas])
}

export async function rpcDestroy()
{
    // Module._js_destroy_context()
}

export async function rpcCall(input) {
    return rpcSend({
        type: "rpc",
        input: JSON.stringify(input),
    })
}

export async function rpcLoadScene(name, blob) {
    return rpcSend({
        type: "rpc-load",
        name, blob,
    })
}

/*

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

export async function rpcSetup()
{
    Module._js_setup()
}

export async function rpcDestroy()
{
    Module._js_destroy_context()
}

export async function rpcCall(input)
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

export async function rpcLoadScene(name, buffer) {
    const size = buffer.byteLength
    const dataPointer = Module._malloc(size)
    if (!dataPointer) throw new Error("Out of memory!")
    HEAPU8.set(new Uint8Array(buffer), dataPointer)
    const scene = await rpcCall({ cmd: "loadScene", name, dataPointer, size })
    Module._free(dataPointer)
    return scene.scene
}

*/
