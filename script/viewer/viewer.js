const Module = window.Module

function deepEqual(a, b) {
    if (a === b) return true
    const type = typeof a
    if (type !== typeof b) return false
    if (type === "number") {
        return Math.abs(a - b) <= 1e-6
    } else if (type === "object") {
        if (a === null) return b === null
        if (b === null) return false
        if (Array.isArray(a)) {
            if (!Array.isArray(b)) return false
            const len = a.length
            if (len != b.length) return false
            for (let i = 0; i < a.length; i++) {
                if (!deepEqual(a[i], b[i])) return false
            }
            return true
        } else {
            for (const key of Object.keys(a)) {
                if (!b.hasOwnProperty(key)) return false
                if (!deepEqual(a[key], b[key])) return false
            }
            for (const key of Object.keys(b)) {
                if (!a.hasOwnProperty(key)) return false
            }
            return true
        }
    } else {
        return a === b
    }
}

let rpcInitialized = false
let viewers = { }
let dirtyQueue = []
let dirtySet = new Set()
let sceneState = { }
let scenesToLoad = new Set()

let renderTargets = []

const symId = Symbol("viewer-id")

let rafToken = null

function log(str) {
    return
    console.log(str)
}

function fetchScene(url) {
    if (!sceneState[url]) {
        scenesToLoad.add(url)
        somethingChanged()
    }
}

function somethingChanged() {
    if (rafToken === null) {
        rafToken = requestAnimationFrame(renderCycle)
    }
}

function markDirty(id) {
    if (dirtySet.has(id)) return
    dirtySet.add(id)
    dirtyQueue.push(id)
    somethingChanged()
}

let maximumResolution = 1024

function getRenderResolution(root) {
    const { clientWidth, clientHeight } = root
    const maxExtent = Math.max(clientWidth, clientHeight)
    let scale = 1.0
    if (maxExtent > maximumResolution) {
        scale = scale = maximumResolution / maxExtent
    }
    return {
        width: Math.round(clientWidth * scale),
        height: Math.round(clientHeight * scale)
    }
}

function updateResolution(viewer) {
    const resolution = getRenderResolution(viewer.root)
    if (!deepEqual(viewer.resolution, resolution)) {
        viewer.resolution = resolution
        markDirty(viewer.id)
    }
}

const resizeObserver = new ResizeObserver(() => {
    for (const viewer of Object.values(viewers)) {
        updateResolution(viewer)
    }
})

export function renderViewer(id, root, desc) {

    // Find or create a viewer state
    let viewer = viewers[id]
    if (!viewer) {
        viewer = { id }
        viewers[id] = viewer
    }

    // Migrate the root if necessary
    if (viewer.root !== root) {
        if (viewer.canvas) {
            viewer.root.removeChild(canvas)
            root.appendChild(canvas)
        }
        if (viewer.img) {
            viewer.root.removeChild(img)
            root.appendChild(img)
        }
        if (viewer.root) {
            resizeObserver.unobserve(viewer.root)
            delete viewer.root[symId]
        }
        viewer.root = root
        resizeObserver.observe(viewer.root)
        root[symId] = id
        updateResolution(viewer)
    }

    // Update the render desc
    if (!deepEqual(viewer.desc, desc)) {
        fetchScene(desc.sceneName)
        viewer.desc = desc
        markDirty(viewer.id)
    }
}

export function removeViewer(id) {
    const viewer = viewers[id]
    if (!viewer) return

    if (viewer.canvas) {
        viewer.canvas.width = 1
        viewer.canvas.height = 1
        viewer.root.removeChild(canvas)
    }
    if (viewer.img) {
        viewer.img.src = ""
        viewer.root.removeChild(img)
    }
    if (viewer.root) {
        resizeObserver.unobserve(viewer.root)
        delete viewer.root[symId]
    }
    delete viewers[id]

    if (dirtySet.has(id)) {
        dirtySet.delete(id)
        const index = dirtyQueue.findIndex(id)
        if (index >= 0) dirtyQueue.splice(index, 1)
    }
}

function readyToRender(id) {
    const viewer = viewers[id]
    if (!viewer) return false
    const desc = viewer.desc
    if (!desc) return false
    if (sceneState[desc.sceneName] !== "loaded") return false
    return true
}

function renderCycle() {
    rafToken = null
    if (!rpcInitialized) return

    if (scenesToLoad.size > 0) {
        for (const url of scenesToLoad) {
            log(`Loading ${url}`)
            sceneState[url] = "loading"
            fetch(url)
                .then(response => response.arrayBuffer())
                .then(buffer => loadSceneFromBuffer(url, buffer))
                .then(() => sceneState[url] = "loaded")
                .catch(() => sceneState[url] = "error")
                .then(somethingChanged)
        }
        scenesToLoad.clear()
    }

    if (renderTargets.length > 0) {
        for (const { id, targetIndex, width, height } of renderTargets) {
            log(`${id}: Compositing`)
            const viewer = viewers[id]
            if (!viewer) continue

            const result = rpcCall({
                cmd: "getPixels",
                targetIndex,
                width,
                height,
            })
            const ptr = result.dataPointer
            const pixels = new Uint8ClampedArray(Module.HEAPU8.buffer, ptr, width*height*4)
            const imageData = new ImageData(pixels, width, height)

            if (viewer.img) {
                viewer.img.src = ""
                viewer.root.removeChild(viewer.img)
                viewer.img = null
            }

            let canvas = viewer.canvas
            if (!canvas) {
                log(`${id}: Creating canvas ${width}x${height}`)
                canvas = document.createElement("canvas")
                canvas.classList.add("ufbx-canvas")
                canvas.width = width
                canvas.height = height
                viewer.root.appendChild(canvas)
                viewer.canvas = canvas
                viewer.ctx = canvas.getContext("2d", {
                    alpha: false,
                })
            }

            if (canvas.width !== width || canvas.height !== height) {
                log(`${id}: Resizing canvas ${width}x${height}`)
                canvas.width = width
                canvas.height = height
            }

            viewer.ctx.putImageData(imageData, 0, 0)
        }
    }

    renderTargets.length = 0

    let reqIx = dirtyQueue.findIndex(readyToRender)
    if (reqIx >= 0) {
        const id = dirtyQueue.splice(reqIx, 1)[0]
        dirtySet.delete(id)
        log(`${id}: Rendering`)
        const viewer = viewers[id]
        const { width, height } = viewer.resolution
        const targetIndex = 0
        rpcCall({
            cmd: "render",
            target: { targetIndex, width, height },
            desc: viewer.desc,
        })
        renderTargets.push({ id, targetIndex, width, height })
        somethingChanged()
    }
}

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

    // console.log(`${input.cmd}: ${end-begin}ms`)

    if (output.error) {
        console.error(output.error)
    }

    return output
}

function loadSceneFromBuffer(name, buffer) {
    const size = buffer.byteLength
    const dataPointer = Module._malloc(size)
    if (!dataPointer) throw new Error("Out of memory!")
    HEAPU8.set(new Uint8Array(buffer), dataPointer)
    rpcCall({ cmd: "loadScene", name, dataPointer, size })
    Module._free(dataPointer)
}

let renderCanvas = null

function initializeNativeViewer() {
    renderCanvas = document.createElement("canvas")
    renderCanvas.id = "ufbx-render-canvas"
    renderCanvas.width = 800
    renderCanvas.height = 600
    document.body.appendChild(renderCanvas)

    Module._js_setup()
    rpcCall({ cmd: "init" })
    rpcInitialized = true
    somethingChanged()
}

export function setupViewers() {
    if (Module.hasRun) {
        initializeNativeViewer()
    } else {
        Module.onRuntimeInitialized = initializeNativeViewer
    }
}
