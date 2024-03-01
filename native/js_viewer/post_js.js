
function rpcOnReady(cb)
{
    if (Module.hasRun) {
        cb()
    } else {
        Module.onRuntimeInitialized = cb
    }
}

function rpcCall(input)
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

function rpcLoadScene(name, blob, cb) {
    blob.arrayBuffer().then((buffer) => {
        const size = buffer.byteLength
        const dataPointer = Module._malloc(size)
        if (!dataPointer) throw new Error("Out of memory!")
        HEAPU8.set(new Uint8Array(buffer), dataPointer)
        const scene = rpcCall({ cmd: "loadScene", name, dataPointer, size })
        Module._free(dataPointer)
        cb(scene.scene)
    })
}

let globalCanvas = null
self.addEventListener("message", (e) => {
    const { token, message } = e.data
    if (message.type === "init") {
        rpcOnReady(() => {
            self.postMessage({ token, result: "ok" })
        })
    } else if (message.type === "setup") {
        const ctx = GL.createContext(message.canvas, {
            powerPreference: "low-power",
            majorVersion: 2,
        })
        globalCanvas = message.canvas
        GL.makeContextCurrent(ctx)
        Module._js_setup()
        self.postMessage({ token, result: "ok" })
    } else if (message.type === "rpc-load") {
        rpcLoadScene(message.name, message.blob, (result) => {
            self.postMessage({ token, result })
        })
    } else if (message.type === "rpc") {
        const input = JSON.parse(message.input)
        if (input.cmd === "render") {
            globalCanvas.width = input.target.width
            globalCanvas.height = input.target.height
        }
        const result = rpcCall(input)
        self.postMessage({ token, result })
    }
})
