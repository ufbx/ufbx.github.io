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

const timeScale = 1.0
function getTime() {
    const time = performance.now() / 1000.0
    return time * timeScale
}

let rpcInitialized = false
let viewers = { }
let dirtyQueue = []
let dirtySet = new Set()
let sceneState = { }
let scenesToLoad = new Set()
let requestedInteractiveId = null
let interactiveId = ""
let prevInteractiveId = null
let allocatedResources = {
    targets: true,
    scenes: true,
    globals: true,
}

let latestRenders = []
let renderTargets = []
let interactiveTarget = null

let idleTimeoutId = null
let lastRenderTime = getTime()

const symId = Symbol("viewer-id")

let animationFrameRequestId = null

function log(str) {
    return;
    console.log(str)
}

function fetchScene(url) {
    if (!sceneState[url]) {
        scenesToLoad.add(url)
        somethingChanged()
    }
}

function somethingChanged() {
    if (animationFrameRequestId === null) {
        animationFrameRequestId = requestAnimationFrame(renderCycle)
    }
    if (idleTimeoutId === null) {
        idleTimeoutId = setTimeout(idleCycle, 1000 / timeScale)
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

export function renderViewer(id, root, desc, opts={}) {

    // Find or create a viewer state
    let viewer = viewers[id]
    if (!viewer) {
        viewer = { id, lastRenderTime: getTime() }
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
        viewer.desc = JSON.parse(JSON.stringify(desc))
        markDirty(viewer.id)
    }

    if (opts.interactive) {
        requestedInteractiveId = id
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

function idleCycle() {
    const currentTime = getTime()
    const timeSinceLastRender = currentTime - lastRenderTime

    if (timeSinceLastRender > 2.0 && interactiveId !== "") {
        console.log("Converting interactive target to non-interactive")
        requestedInteractiveId = ""
        somethingChanged()
    }

    if (interactiveId === "") {
        if (timeSinceLastRender > 5.0 && allocatedResources.targets) {
            allocatedResources.targets = false
            console.log("Freeing targets")
            rpcCall({
                cmd: "freeResources",
                targets: true,
            })
        }
        if (timeSinceLastRender > 10.0 && allocatedResources.scenes) {
            allocatedResources.scenes = false
            console.log("Freeing scenes")
            rpcCall({
                cmd: "freeResources",
                scenes: true,
            })
        }
    }

    let allUnloaded = true
    for (const id in viewers) {
        const viewer = viewers[id]
        if (!viewer.canvas) continue
        const timeSinceLastViewerRender = currentTime - viewer.lastRenderTime
        if (timeSinceLastViewerRender < 5.0) continue

        console.log(`Converting ${viewer.id} to <img>`)

        const url = viewer.canvas.toDataURL()

        const img = document.createElement("img")
        img.classList.add("ufbx-canvas")
        viewer.img = img
        viewer.root.appendChild(img)

        const canvas = viewer.canvas
        viewer.canvas = null

        img.onload = () => {
            // Firefox bug
            setTimeout(() => {
                canvas.width = 1
                canvas.height = 1
                viewer.root.removeChild(canvas)
            }, 100)
        }

        img.src = url

        allUnloaded = false
        break
    }

    if (allUnloaded && timeSinceLastRender > 120.0 && allocatedResources.globals) {
            console.log("Dropping context")
            allocatedResources.targets = false
            allocatedResources.scenes = false
            allocatedResources.globals = false
            rpcCall({
                cmd: "freeResources",
                targets: true,
                scenes: true,
                globals: true,
            })
            Module._js_destroy_context()

            const ext = GLctx.getExtension('WEBGL_lose_context')
            if (ext) ext.loseContext()

            globalRenderCanvas.width = 1
            globalRenderCanvas.height = 1
            document.body.removeChild(globalRenderCanvas)
            globalRenderCanvas = null
    }

    idleTimeoutId = setTimeout(idleCycle, 1000 / timeScale)
}

function renderCycle() {
    animationFrameRequestId = null
    if (!rpcInitialized) return
    const renderCanvas = getRenderCanvas()

    if (!allocatedResources.globals) {
        allocatedResources.globals = true
        Module._js_reload_context()
        rpcCall({ cmd: "init" })
    }

    const currentTime = getTime()

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

    if (!requestedInteractiveId) {
        const cutoffTime = currentTime - 0.5
        latestRenders = latestRenders.filter(r => r.time >= cutoffTime)
        const renderCounts = { }
        for (const { id } of latestRenders) {
            renderCounts[id] = (renderCounts[id] ?? 0) + 1
        }

        let interactiveCount = renderCounts[interactiveId] ?? 0
        let maxCount = 0
        let maxId = 0
        for (let id in renderCounts) {
            const count = renderCounts[id]
            if (count > maxCount) {
                maxCount = count
                maxId = id
            }
        }

        if (maxCount >= 2 && maxCount >= interactiveCount * 1.5) {
            requestedInteractiveId = maxId
        }
    }

    if (requestedInteractiveId !== null) {
        if (interactiveId !== requestedInteractiveId) {
            if (!renderTargets.some(t => t.id === interactiveId)) {
                if (interactiveTarget && interactiveTarget.id === interactiveId) {
                    log(`${interactiveId}: Compositing from interactive ${interactiveTarget.targetIndex}`)
                    renderTargets.push(interactiveTarget)
                }
            }
            interactiveId = requestedInteractiveId
            renderCanvas.style.opacity = "0%"
        }
        requestedInteractiveId = null
        interactiveTarget = null
    }

    if (renderTargets.length > 0) {
        for (const { id, targetIndex, width, height } of renderTargets) {
            const viewer = viewers[id]
            if (!viewer) continue

            if (interactiveId === id) {
                if (prevInteractiveId === id) {
                    viewer.canvas.style.opacity = "0%"
                    renderCanvas.style.opacity = "100%"
                    continue
                } else {
                    prevInteractiveId = interactiveId
                }
            }

            log(`${id}: Compositing from ${targetIndex}`)

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

            canvas.style.opacity = "100%"
            if (canvas.width !== width || canvas.height !== height) {
                log(`${id}: Resizing canvas ${width}x${height}`)
                canvas.width = width
                canvas.height = height
            }

            viewer.ctx.putImageData(imageData, 0, 0)
        }
    }

    renderTargets.length = 0

    let idsToRender = []

    if (dirtySet.has(interactiveId) && readyToRender(interactiveId)) {
        idsToRender.push(interactiveId)
        let interactiveIx = dirtyQueue.indexOf(interactiveId)
        dirtyQueue.splice(interactiveIx, 1)
        dirtySet.delete(interactiveId)
    }

    let reqIx = dirtyQueue.findIndex(readyToRender)
    if (reqIx >= 0) {
        const id = dirtyQueue[reqIx]
        idsToRender.push(id)
        dirtyQueue.splice(reqIx, 1)
        dirtySet.delete(id)
    }

    for (const id of idsToRender) {
        const viewer = viewers[id]
        const { width, height } = viewer.resolution

        const interactive = id === interactiveId
        const targetIndex = interactive ? 1 : 0
        const samples = 4

        log(`${id}: Rendering to ${targetIndex}`)

        rpcCall({
            cmd: "render",
            target: { targetIndex, width, height, samples },
            desc: viewer.desc,
        })

        const target = { id, targetIndex, width, height }
        if (interactive) {
            log(`${id}: Presenting ${targetIndex}`)
            rpcCall({
                cmd: "present",
                targetIndex,
                width,
                height,
            })

            const rect = viewer.root.getBoundingClientRect()
            renderCanvas.style.left = `${rect.left}px`
            renderCanvas.style.top = `${rect.top}px`
            renderCanvas.style.width = `${rect.right-rect.left}px`
            renderCanvas.style.height = `${rect.bottom-rect.top}px`
            renderCanvas.width = width
            renderCanvas.height = height
            interactiveTarget = target
        }

        viewer.lastRenderTime = currentTime
        latestRenders.push({ id, time: currentTime })
        renderTargets.push(target)
    }

    lastRenderTime = currentTime
    allocatedResources.scenes = true
    allocatedResources.targets = true

    if (idsToRender.length > 0) {
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

let globalRenderCanvas = null

function getRenderCanvas()
{
    let recreate = false
    if (allocatedResources.globals && window.GLctx && window.GLctx.isContextLost()) {
        console.log("Recreating lost context")

        rpcCall({
            cmd: "freeResources",
            scenes: true,
            targets: true,
            globals: true,
        })

        Module._js_destroy_context()
        recreate = true
    }

    if (globalRenderCanvas) return globalRenderCanvas
    console.log("Creating new canvas")
    const renderCanvas = document.createElement("canvas")
    renderCanvas.id = "ufbx-render-canvas"
    renderCanvas.width = 800
    renderCanvas.height = 600
    renderCanvas.style.position = "absolute"
    renderCanvas.style.opacity = "0%"
    document.body.appendChild(renderCanvas)
    globalRenderCanvas = renderCanvas

    if (recreate || !allocatedResources.globals) {
        allocatedResources.globals = true
        Module._js_reload_context()
        rpcCall({ cmd: "init" })
    }

    return renderCanvas
}

function initializeNativeViewer() {
    getRenderCanvas()
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
