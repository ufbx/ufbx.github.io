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
let sceneInfos = { }
let sceneInfoListeners = []
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

let maximumResolution = 2048

function getRenderResolution(root) {
    const { width, height } = root.getBoundingClientRect()
    const elementWidth = Math.ceil(width)
    const elementHeight = Math.ceil(height)
    const maxExtent = Math.max(elementWidth, elementHeight)
    let scale = 1.0
    if (maxExtent > maximumResolution) {
        scale = scale = maximumResolution / maxExtent
    }
    return {
        elementWidth, elementHeight,
        renderWidth: Math.round(elementWidth * scale),
        renderHeight: Math.round(elementHeight * scale)
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

export function queryResolution(id) {
    const viewer = viewers[id]
    return viewer?.resolution
}

export function addSceneInfoListener(cb) {
    sceneInfoListeners.push(cb)
    for (const name in sceneInfos) {
        cb(name, sceneInfos[name])
    }
}

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
        if (timeSinceLastRender > 10.0 && allocatedResources.targets) {
            allocatedResources.targets = false
            console.log("Freeing targets")
            rpcCall({
                cmd: "freeResources",
                targets: true,
            })
        }
        if (timeSinceLastRender > 20.0 && allocatedResources.scenes) {
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
        const canvas = viewer.canvas

        const timeSinceLastViewerRender = currentTime - viewer.lastRenderTime
        if (timeSinceLastViewerRender < 30.0) continue

        console.log(`Converting ${viewer.id} to <img>`)

        const url = viewer.canvas.toDataURL()

        const img = document.createElement("img")
        img.classList.add("ufbx-canvas")
        img.style.width = canvas.style.width
        img.style.height = canvas.style.height
        viewer.img = img
        viewer.root.appendChild(img)

        const root = viewer.root
        viewer.canvas = null

        img.onload = () => {
            // Firefox bug
            setTimeout(() => {
                canvas.width = 1
                canvas.height = 1
                root.removeChild(canvas)
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
        for (const { id, targetIndex, renderWidth, renderHeight, elementWidth, elementHeight } of renderTargets) {
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
                width: renderWidth,
                height: renderHeight,
            })
            const ptr = result.dataPointer
            const pixels = new Uint8ClampedArray(Module.HEAPU8.buffer, ptr, renderWidth*renderHeight*4)
            const imageData = new ImageData(pixels, renderWidth, renderHeight)

            if (viewer.img) {
                viewer.img.src = ""
                viewer.root.removeChild(viewer.img)
                viewer.img = null
            }

            let canvas = viewer.canvas
            if (!canvas) {
                log(`${id}: Creating canvas ${renderWidth}x${renderHeight}`)
                canvas = document.createElement("canvas")
                canvas.classList.add("ufbx-canvas")
                canvas.width = renderWidth
                canvas.height = renderHeight
                canvas.style.width = `${elementWidth}px`
                canvas.style.height = `${elementHeight}px`
                viewer.root.appendChild(canvas)
                viewer.canvas = canvas
                viewer.ctx = canvas.getContext("2d", {
                    alpha: false,
                })
            }

            canvas.style.opacity = "100%"
            if (canvas.width !== renderWidth || canvas.height !== renderHeight) {
                log(`${id}: Resizing canvas ${renderWidth}x${renderHeight}`)
                canvas.width = renderWidth
                canvas.height = renderHeight
            }
            canvas.style.width = `${elementWidth}px`
            canvas.style.height = `${elementHeight}px`

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
        const { renderWidth, renderHeight, elementWidth, elementHeight } = viewer.resolution

        const interactive = id === interactiveId
        const targetIndex = interactive ? 1 : 0
        const samples = 4

        log(`${id}: Rendering to ${targetIndex}`)

        rpcCall({
            cmd: "render",
            target: { targetIndex, width: renderWidth, height: renderHeight, samples },
            desc: viewer.desc,
        })

        const target = { id, targetIndex, renderWidth, renderHeight, elementWidth, elementHeight }
        if (interactive) {
            log(`${id}: Presenting ${targetIndex}`)
            const rect = viewer.root.getBoundingClientRect()
            renderCanvas.style.left = `${rect.left}px`
            renderCanvas.style.top = `${rect.top}px`
            renderCanvas.style.width = `${elementWidth}px`
            renderCanvas.style.height = `${elementHeight}px`
            if (renderCanvas.width !== renderWidth || renderCanvas.height !== renderHeight) {
                log(`${id}: Resizing render canvas ${renderWidth}x${renderHeight}`)
                renderCanvas.width = renderWidth
                renderCanvas.height = renderHeight
            }
            interactiveTarget = target

            rpcCall({
                cmd: "present",
                targetIndex,
                width: renderWidth,
                height: renderHeight,
            })
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
    const scene = rpcCall({ cmd: "loadScene", name, dataPointer, size })
    Module._free(dataPointer)
    const info = scene.scene
    if (info) {
        sceneInfos[name] = info
        for (const cb of sceneInfoListeners) {
            cb(name, info)
        }
    }
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
    renderCanvas.width = 10
    renderCanvas.height = 10
    renderCanvas.style.position = "absolute"
    renderCanvas.style.top = "0px"
    renderCanvas.style.left = "0px"
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
