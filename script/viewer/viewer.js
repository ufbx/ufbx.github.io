const Module = window.Module

function rpcCall(input) {
    // console.log(">", input)
    const inStr = JSON.stringify(input)
    const inSize = inStr.length * 4 + 1
    const inPtr = Module._malloc(inSize)
    stringToUTF8(inStr, inPtr, inSize)

    const begin = performance.now()
    const outPtr = Module._js_rpc(inPtr)
    const end = performance.now()

    const outStr = UTF8ToString(outPtr)
    Module._free(outPtr)
    const output = JSON.parse(outStr)
    // console.log("<", output)

    console.log(`${input.cmd}: ${end-begin}ms`)

    if (output.error) {
        console.error(output.error)
    }

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

    var time = 0
    var count = 0
    var phase = 0

    function render() {
        time += 0.005
        phase++

        if (phase % 2 == 1) {
            rpcCall({
                cmd: "render",
                target: {
                    width: 800,
                    height: 600,
                    samples: 8,
                },
                desc: {
                    sceneName: "barbarian",
                    camera: {
                        position: [Math.sin(time)*20, 10, Math.cos(time)*20],
                        target: [0, 3, 0],
                        fieldOfView: 30,
                    },
                },
            })

            rpcCall({
                cmd: "present",
                width: 800,
                height: 600,
            })
        } else {
            const result = rpcCall({
                cmd: "getPixels",
                width: 800,
                height: 600,
            })
            const ptr = result.dataPointer
            const pixels = new Uint8ClampedArray(Module.HEAPU8.buffer, ptr, 800*600*4)
            const imageData = new ImageData(pixels, 800, 600)

            const canvas = document.querySelector("#copy-canvas")
            const ctx = canvas.getContext("2d", {
                alpha: false,
            })
            ctx.putImageData(imageData, 0, 0)
        }

        if (++count < 60) {
            window.setTimeout(render, 100)
        }
    }

    fetch("/static/models/barbarian.fbx")
        .then(response => response.arrayBuffer())
        .then(buffer => loadScene("barbarian", buffer))
        .then(() => render())
}

export function setupViewer() {
    if (Module.hasRun) {
        initializeNativeViewer()
    } else {
        Module.onRuntimeInitialized = initializeNativeViewer
    }
}
