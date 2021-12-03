import { renderRoot, setScene } from "./components"

renderRoot(document.getElementById("root"))

const renderCanvas = document.getElementById("render-canvas")
let activeCanvas = null
let usedCanvas = null

function copyCanvas(dst, src)
{
    const fbIndex = Module._viewer_get_fb()

    const width = src.width
    const height = src.height
    const buffer = new Uint8ClampedArray(width * height * 4)
    const gl = src.getContext("webgl2")

    const prevFb = gl.getParameter(gl.FRAMEBUFFER_BINDING)
    const fb = Module.GL.framebuffers[fbIndex]
    gl.bindFramebuffer(gl.FRAMEBUFFER, fb)
    gl.readPixels(0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE, buffer)
    gl.bindFramebuffer(gl.FRAMEBUFFER, prevFb)

    const image = new ImageData(buffer, width, height)
    const ctx = dst.getContext("2d", { alpha: false })
    ctx.putImageData(image, 0, 0, 0, 0, width, height)
}

function moveCanvas(targetCanvas)
{
    if (targetCanvas == activeCanvas) return
    if (activeCanvas) {
        copyCanvas(activeCanvas, renderCanvas)
    }

    const { x, y, width, height } = targetCanvas.getBoundingClientRect()
    activeCanvas = targetCanvas
    renderCanvas.style.transform = `translate(${x}px, ${y}px)`
}

for (const canvas of document.querySelectorAll(".potential-canvas")) {
    canvas.addEventListener("mousemove", e => {
        moveCanvas(canvas)
    })
}

function viewerPaused()
{
}

function viewerResumed()
{
}

Module.onRuntimeInitialized = () => {
    const pauseCb = addFunction(viewerPaused, "v")
    const resumeCb = addFunction(viewerResumed, "v")
    Module._viewer_init(pauseCb, resumeCb)
}

const fileInput = document.getElementById("file-input")
fileInput.addEventListener("change", (e) => {
    const file = fileInput.files[0]
    const reader = new FileReader()
    reader.onload = (e) => {
        const buffer = e.target.result
        const size = buffer.byteLength
        const data = Module._malloc(size)
        if (!data) {
            console.error("Out of memory!")
            return
        }
        HEAPU8.set(new Uint8Array(buffer), data)
        const resultPtr = Module._load_scene(data, size)

        if (resultPtr) {
            const json = UTF8ToString(resultPtr)
            const scene = JSON.parse(json)
            console.log(scene)
            setScene(scene)
        }

        Module._free(resultPtr)
        Module._free(data)
    }
    reader.readAsArrayBuffer(file)
}, false)