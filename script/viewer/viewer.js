const Module = window.Module

function rpcCall(input) {
    console.log(">", input)
    const inStr = JSON.stringify(input)
    const inSize = inStr.length * 4 + 1
    const inPtr = Module._malloc(inSize)
    stringToUTF8(inStr, inPtr, inSize)
    const outPtr = Module._js_rpc(inPtr)
    const outStr = UTF8ToString(outPtr)
    Module._free(outPtr)
    const output = JSON.parse(outStr)
    console.log("<", output)
    return output
}

function loadScene(name, buffer) {
    const size = buffer.byteLength
    const dataPointer = Module._malloc(size)
    if (!dataPointer) throw new Error("Out of memory!")
    HEAPU8.set(new Uint8Array(buffer), dataPointer)
    rpcCall({ cmd: "loadScene", name, dataPointer, size })
    Module._free(dataPointer)
}

function initializeNativeViewer() {
    Module._js_setup()
    rpcCall({ cmd: "init" })

    fetch("/static/models/barbarian.fbx")
        .then(response => response.arrayBuffer())
        .then(buffer => loadScene("barbarian", buffer))
        .then(() => {
            rpcCall({
                cmd: "render",
                target: {
                    width: 800,
                    height: 600,
                    samples: 4,
                },
                desc: {
                    sceneName: "barbarian",
                    camera: {
                        position: [-8, 10, 20],
                        target: [0, 3, 0],
                        fieldOfView: 30,
                    },
                },
            })
        })
        .then(() => {
            rpcCall({
                cmd: "present",
                width: 800,
                height: 600,
            })
        })
}

export function setupViewer() {
    if (Module.hasRun) {
        initializeNativeViewer()
    } else {
        Module.onRuntimeInitialized = initializeNativeViewer
    }
}
