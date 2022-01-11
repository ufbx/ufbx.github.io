import { rpcCall, rpcMemory, rpcGl, rpcSetup, rpcReload, rpcOnReady, rpcLoadScene, rpcDestroy } from "./rpc.js"
import { deepEqual, getTime } from "../common"

type SceneDesc = {
    sceneName: string
    latestInteractionTime: number
    [rest: string]: any
}

type Resolution = {
    readonly width: number
    readonly height: number
}

type EmptyViewer = {
    readonly state: "empty"
}

type ImageViewer = {
    readonly state: "image"
    image: HTMLImageElement
}

type CanvasViewer = {
    readonly state: "canvas"
    canvas: HTMLCanvasElement
    ctx: ImageBitmapRenderingContext
    canvasResolution: Resolution
    renderResolution: Resolution
}

type RealtimeViewer = {
    readonly state: "realtime"
    realtimeCanvas: HTMLCanvasElement
    previousCanvas: HTMLCanvasElement | null
    canvasResolution: Resolution
    renderResolution: Resolution
}

type ViewerState = EmptyViewer | ImageViewer | CanvasViewer | RealtimeViewer
type ViewerTag = ViewerState["state"]

type Viewer = {
    id: string,
    root: HTMLElement
    rootResolution: Resolution
    desc: SceneDesc
    dirty: boolean
    state: ViewerState
    lastRenderTime: number
    prevRenderTime: number
}

type SceneState = "unloaded" | "loading" | "loaded" | "error"
type SceneInfo = any

type SceneInfoListener = (name: string, info: SceneInfo) => void

type Scene = {
    state: SceneState
    info: SceneInfo | null
}

function detach(element: HTMLElement) {
    if (element.parentElement) {
        element.parentElement.removeChild(element)
    }
}

function destroy(element: HTMLElement) {
    detach(element)
    if (element instanceof HTMLCanvasElement) {
        element.width = 1
        element.height = 1
    } else if (element instanceof HTMLImageElement) {
        element.onload = null
        element.onerror = null
        element.src = "#"
    }
}

let globalSyncLocked: boolean = false
let lockedTargets: Set<number> = new Set()
let lockedViewers: Set<string> = new Set()
let frameCallbacks: Array<() => boolean> = []
let frameToken: number | null = null
let fastIdleToken: number | null = null
let slowIdleToken: number | null = null
let viewers: Map<string, Viewer> = new Map()
let globalRealtimeCanvas: HTMLCanvasElement | null = null
let rpcInitialized: boolean = false
let rpcDestroyed: boolean = false
let scenes: Map<string, Scene> = new Map()
let scenesToLoad: Array<string> = []
let sceneInfoListeners: Array<SceneInfoListener> = []
let realtimeViewerId: string = ""
let prevInteractedViewerId: string = ""
let prevInteractedViewerTime: number = -100.0
let interactedViewerId: string = ""
let interactedViewerTime: number = -100.0
let idleStage: number = 0

function loadScene(path: string) {
    if (scenes.has(path)) return
    scenes.set(path, {
        state: "unloaded",
        info: null,
    })
    scenesToLoad.push(path)
    requestFrame()
}

function readyToRender(viewer: Viewer) {
    const desc = viewer.desc
    const scene = scenes.get(desc.sceneName)
    if (!scene) return false
    if (scene.state === "error") return true
    if (scene.state !== "loaded") return false
    return true
}

function getRealtimeCanvas(): HTMLCanvasElement {
    const gl = rpcGl()
    if (gl && gl.isContextLost()) {
        console.log("Recreating lost context")
        if (!rpcDestroyed) {
            rpcCall({
                cmd: "freeResources",
                scenes: true,
                targets: true,
                globals: true,
            })
            rpcDestroy()
            rpcDestroyed = true
        }
    }

    if (rpcDestroyed) {
        if (globalRealtimeCanvas) {
            destroy(globalRealtimeCanvas)
        }
        globalRealtimeCanvas = null
        rpcDestroyed = false
    }
    
    if (globalRealtimeCanvas) return globalRealtimeCanvas
    const canvas = document.createElement("canvas")
    canvas.id = "ufbx-render-canvas"
    canvas.width = 1
    canvas.height = 1
    canvas.style.display = "none"
    globalRealtimeCanvas = canvas
    document.body.appendChild(canvas)

    rpcSetup()
    rpcCall({ cmd: "init" })

    return canvas
}

function requestIdle() {
    idleStage = 0
    if (slowIdleToken !== null) {
        window.clearTimeout(slowIdleToken)
        slowIdleToken = null
    }
    if (fastIdleToken === null) {
        fastIdleToken = window.setTimeout(idleTick, 100)
    }
}

function requestFrame() {
    if (frameToken === null) {
        frameToken = window.requestAnimationFrame(animationFrame)
    }
    requestIdle()
}

function frameCallback(cb: () => boolean) {
    frameCallbacks.push(cb)
    requestFrame()
}

async function waitForExistingSync() {
    if (!globalSyncLocked) return
    return new Promise((resolve) => {
        frameCallback(() => {
            if (!globalSyncLocked) {
                resolve(null)
                return false
            } else {
                return true
            }
        })
    })
}

async function finishRendering() {
    if (!globalSyncLocked) {
        const gl = rpcGl()
        const sync = gl.fenceSync(gl.SYNC_GPU_COMMANDS_COMPLETE, 0)
        if (!sync) return
        globalSyncLocked = true
        frameCallback(() => {
            const res = gl.clientWaitSync(sync, gl.SYNC_FLUSH_COMMANDS_BIT, 0)
            if (res === gl.TIMEOUT_EXPIRED) return true
            globalSyncLocked = false
            gl.deleteSync(sync)
            return false
        })
    }
    await waitForExistingSync()
}

async function acquireTarget<T>(targetIndex: number, cb: () => Promise<T>): Promise<T> {
    return new Promise((resolve, reject) => {
        const inner = () => {
            if (lockedTargets.has(targetIndex)) return true
            lockedTargets.add(targetIndex)
            cb().then((res) => {
                lockedTargets.delete(targetIndex)
                resolve(res)
            }).catch((err) => {
                lockedTargets.delete(targetIndex)
                reject(err)
            })
            return false
        }

        if (inner()) {
            frameCallback(inner)
        }
    })
}

async function acquireViewer<T>(id: string, cb: () => Promise<T>): Promise<T> {
    return new Promise((resolve, reject) => {
        const inner = () => {
            if (lockedViewers.has(id)) return true
            lockedViewers.add(id)
            cb().then((res) => {
                lockedViewers.delete(id)
                resolve(res)
            }).catch((err) => {
                lockedViewers.delete(id)
                reject(err)
            })
            return false
        }

        if (inner()) {
            frameCallback(inner)
        }
    })
}

async function renderToTarget(targetIndex: number, desc: SceneDesc, resolution: Resolution, pixelScale: number) {
    await waitForExistingSync()

    const samples = 4
    rpcCall({
        cmd: "render",
        target: { targetIndex, width: resolution.width, height: resolution.height, samples, pixelScale },
        desc,
    })
}

function presentTarget(targetIndex: number, resolution: Resolution) {
    rpcCall({
        cmd: "present",
        targetIndex,
        width: resolution.width,
        height: resolution.height,
    })
}

async function renderToBitmap(targetIndex: number, desc: SceneDesc, resolution: Resolution, pixelScale: number) {
    return acquireTarget(0, async () => {
        await renderToTarget(0, desc, resolution, pixelScale)
        await finishRendering()
        const result = rpcCall({
            cmd: "getPixels",
            targetIndex,
            width: resolution.width,
            height: resolution.height,
        })
        const ptr = result.dataPointer
        const pixels = new Uint8ClampedArray(rpcMemory(), ptr, resolution.width*resolution.height*4)
        const imageData = new ImageData(pixels, resolution.width, resolution.height)
        return createImageBitmap(imageData)
    })
}

function getRenderResolution(resolution: Resolution, realtime: boolean): Resolution {
    let { width, height } = resolution

    const ratio = Math.max(1.0, window.devicePixelRatio)
    if (!realtime) {
        width *= ratio
        height *= ratio
    }

    return { width: Math.round(width), height: Math.round(height) }
    /*
    if (realtime) {
        return { width: resolution.width / 4 | 0, height: resolution.height / 4 | 0 }
    } else {
        return resolution
    }
    */
}

function resolutionEqual(a: Resolution, b: Resolution) {
    return a.width === b.width && a.height === b.height
}

function setRenderResolution(canvas: HTMLCanvasElement, resolution: Resolution) {
    canvas.width = resolution.width
    canvas.height = resolution.height
}

function setElementResolution(canvas: HTMLElement, resolution: Resolution) {
    canvas.style.width = `${resolution.width}px`
    canvas.style.height = `${resolution.height}px`
}

function transitionToEmpty(viewer: Viewer) {
    const state = viewer.state
    const source = state.state
    if (source === "image") {
        destroy(state.image)
    } else if (source === "canvas") {
        destroy(state.canvas)
    } else if (source === "realtime") {
        detach(state.realtimeCanvas)
        if (state.previousCanvas) {
            destroy(state.previousCanvas)
        }
        realtimeViewerId = ""
    }
    viewer.state = { state: "empty" }
}

async function transitionViewer(viewer: Viewer, target: ViewerTag) {
    const state = viewer.state
    const source = state.state
    if (source === target) return

    if (target === "empty") {
        transitionToEmpty(viewer)
    } else if (target === "image") {
        await transitionViewer(viewer, "canvas")
        const state = viewer.state
        const source = state.state
        if (source !== "canvas") {
            transitionToEmpty(viewer)
            return
        }

        return new Promise((resolve) => {
            const image = document.createElement("img")
            image.classList.add("ufbx-canvas")
            setElementResolution(image, state.canvasResolution)
            image.onload = () => {
                viewer.root.appendChild(image)
                viewer.state = {
                    state: "image",
                    image,
                }
                window.setTimeout(() => {
                    destroy(state.canvas)
                    resolve(null)
                }, 60)
            }
            image.onerror = () => {
                transitionToEmpty(viewer)
                resolve(null)
            }
            image.src = state.canvas.toDataURL()
        })
    } else if (target === "canvas") {
        let canvas = null
        if (source === "realtime") {
            canvas = state.previousCanvas
        }

        if (!canvas) {
            canvas = document.createElement("canvas")
            canvas.classList.add("ufbx-canvas")
        }

        const canvasResolution = viewer.rootResolution
        const renderResolution = getRenderResolution(canvasResolution, false)
        const pixelScale = renderResolution.width / canvasResolution.width

        setRenderResolution(canvas, renderResolution)
        setElementResolution(canvas, canvasResolution)

        const ctx = canvas.getContext("bitmaprenderer")!
        const bitmap = await renderToBitmap(0, viewer.desc, renderResolution, pixelScale)
        ctx.transferFromImageBitmap(bitmap)

        if (source === "realtime") {
            viewer.root.removeChild(state.realtimeCanvas)
        } else {
            transitionToEmpty(viewer)
        }

        viewer.root.appendChild(canvas)

        viewer.state = {
            state: "canvas",
            canvas, ctx, renderResolution, canvasResolution,
        }

        if (source === "realtime") {
            realtimeViewerId = ""
        }

    } else if (target === "realtime") {
        const canvasResolution = viewer.rootResolution
        const renderResolution = getRenderResolution(canvasResolution, true)
        const pixelScale = renderResolution.width / canvasResolution.width

        const canvas = getRealtimeCanvas()
        if (canvas.style.display !== "block") {
            canvas.style.display = "block"
        }

        setRenderResolution(canvas, renderResolution)
        setElementResolution(canvas, canvasResolution)

        await renderToTarget(1, viewer.desc, renderResolution, pixelScale)
        presentTarget(1, renderResolution)

        let previousCanvas = null
        if (source === "canvas") {
            previousCanvas = state.canvas
            detach(previousCanvas)
        } else {
            transitionToEmpty(viewer)
        }

        viewer.root.appendChild(canvas)
        viewer.state = {
            state: "realtime",
            canvasResolution, renderResolution, previousCanvas,
            realtimeCanvas: canvas,
        }
    }
}

async function refreshViewer(viewer: Viewer) {
    const state = viewer.state

    if (state.state === "canvas") {
        const canvas = state.canvas
        const canvasResolution = viewer.rootResolution
        const renderResolution = getRenderResolution(canvasResolution, false)
        const pixelScale = renderResolution.width / canvasResolution.width

        const bitmap = await renderToBitmap(0, viewer.desc, renderResolution, pixelScale)

        if (!resolutionEqual(renderResolution, state.renderResolution)) {
            setRenderResolution(canvas, renderResolution)
            state.renderResolution = renderResolution
        }
        if (!resolutionEqual(canvasResolution, state.canvasResolution)) {
            setElementResolution(canvas, canvasResolution)
            state.canvasResolution = canvasResolution
        }

        const ctx = canvas.getContext("bitmaprenderer")!
        ctx.transferFromImageBitmap(bitmap)
    } else if (state.state === "realtime") {
        const canvas = state.realtimeCanvas
        const canvasResolution = viewer.rootResolution
        const renderResolution = getRenderResolution(canvasResolution, true)
        const pixelScale = renderResolution.width / canvasResolution.width

        await renderToTarget(1, viewer.desc, renderResolution, pixelScale)

        if (!resolutionEqual(renderResolution, state.renderResolution)) {
            setRenderResolution(canvas, renderResolution)
            state.renderResolution = renderResolution
        }
        if (!resolutionEqual(canvasResolution, state.canvasResolution)) {
            setElementResolution(canvas, canvasResolution)
            state.canvasResolution = canvasResolution
        }

        presentTarget(1, renderResolution)
    } else {
        await transitionViewer(viewer, "canvas")
    }
}

function loadSceneFromBuffer(name: string, buffer: ArrayBuffer) {
    const info = rpcLoadScene(name, buffer)
    if (info) {
        const scene = scenes.get(name)!
        scene.info = info
        for (const cb of sceneInfoListeners) {
            cb(name, info)
        }
    }
}

function animationFrame() {
    frameToken = null
    if (!rpcInitialized) return
    const time = getTime()

    const gl = rpcGl()
    if ((gl && gl.isContextLost()) || rpcDestroyed) {
        getRealtimeCanvas()
        for (const viewer of viewers.values()) {
            acquireViewer(viewer.id, async () => {
                viewer.dirty = true
                if (viewer.state.state === "realtime") {
                    await transitionViewer(viewer, "canvas")
                } else {
                    await refreshViewer(viewer)
                }
            })
        }
    }

    if (scenesToLoad.length > 0) {
        for (const url of scenesToLoad) {
            const scene = scenes.get(url)!
            scene.state = "loading"
            fetch(url)
                .then(response => response.arrayBuffer())
                .then(buffer => loadSceneFromBuffer(url, buffer))
                .then(() => scene.state = "loaded")
                .catch(() => scene.state = "error")
                .then(requestFrame)
        }
        scenesToLoad = []
    }

    frameCallbacks = frameCallbacks.filter(cb => cb())
    if (frameCallbacks.length > 0) {
        requestFrame()
    }

    if (time - interactedViewerTime > 1.0) {
        interactedViewerId = ""
    }

    if (interactedViewerId !== "") {
        for (const viewer of viewers.values()) {
            if (viewer.state.state === "realtime" && viewer.id !== interactedViewerId) {
                acquireViewer(viewer.id, async () => {
                    if (viewer.state.state === "realtime") {
                        await transitionViewer(viewer, "canvas")
                    }
                })
                break
            }
        }
    }

    for (const viewer of viewers.values()) {
        if (!viewer.dirty) continue
        if (lockedViewers.has(viewer.id) || !readyToRender(viewer)) {
            requestFrame()
            continue
        }
        acquireViewer(viewer.id, async () => {
            viewer.dirty = false
            const prevRenderTime = viewer.prevRenderTime
            viewer.prevRenderTime = viewer.lastRenderTime
            viewer.lastRenderTime = time

            let promoteToRealtime = false
            if (realtimeViewerId === "" && viewer.state.state !== "realtime") {
                if (interactedViewerId === viewer.id) {
                    promoteToRealtime = true
                } else if (interactedViewerId === "" && time - prevRenderTime < 0.5) {
                    promoteToRealtime = true
                }
            }

            if (promoteToRealtime) {
                realtimeViewerId = viewer.id
                await transitionViewer(viewer, "realtime")
            } else {
                await refreshViewer(viewer)
            }
        })
    }
}

function idleTick() {
    if (!rpcInitialized) return
    slowIdleToken = null
    fastIdleToken = null

    const time = getTime()
    idleStage++

    for (const viewer of viewers.values()) {
        const state = viewer.state

        // Don't let the idle stage grow too large until all viewers are images
        if (state.state !== "image" && idleStage > 10) {
            idleStage = 10
        }

        if (lockedViewers.has(viewer.id)) continue
        const idleTime = time - viewer.lastRenderTime

        if (state.state === "realtime" && idleTime > 0.5) {
            acquireViewer(viewer.id, async () => {
                await transitionViewer(viewer, "canvas")
            }).catch((err) => {
                console.error("Failed to transition from realtime to canvas", err)
                transitionToEmpty(viewer)
            })
        } else if (state.state === "canvas" && idleTime > 10.0) {
            acquireViewer(viewer.id, async () => {
                await transitionViewer(viewer, "image")
            }).catch((err) => {
                console.error("Failed to transition from canvas to image", err)
                transitionToEmpty(viewer)
            })
            break
        }
    }

    if (idleStage == 20) {
        console.log("Freeing targets")
        rpcCall({
            cmd: "freeResources",
            targets: true,
        })
    }

    if (idleStage == 22) {
        console.log("Freeing scenes")
        rpcCall({
            cmd: "freeResources",
            scenes: true,
        })
    }

    if (idleStage == 25) {
        console.log("Dropping globals")
        rpcCall({
            cmd: "freeResources",
            targets: true,
            scenes: true,
            globals: true,
        })
        rpcDestroy()
        rpcDestroyed = true

        const gl = rpcGl()
        if (gl) {
            const ext = gl.getExtension('WEBGL_lose_context')
            if (ext) {
                console.log("Dropping context")
                ext.loseContext()
            }
        }

        if (globalRealtimeCanvas) {
            destroy(globalRealtimeCanvas)
        }
    }

    if (slowIdleToken === null && idleStage < 25) {
        let interval = 100
        if (idleStage >= 20) {
            interval = 10000
        } else if (idleStage >= 10) {
            interval = 1000
        }
        slowIdleToken = window.setTimeout(idleTick, interval)
    }
}

function getElementResolution(element: HTMLElement) {
    const { width, height } = element.getBoundingClientRect()
    return { width: Math.ceil(width), height: Math.ceil(height) }
}

function markDirty(viewer: Viewer) {
    viewer.dirty = true
    requestFrame()
}

function listenToPixelRatio(cb: (ratio: number) => void) {
    function onChange() {
        const ratio = window.devicePixelRatio
        cb(ratio)
        console.log("CAHNGE")
        matchMedia(`(resolution: ${ratio}dppx)`).addEventListener("change", onChange, { once: true })
    }

    const ratio = window.devicePixelRatio
    matchMedia(`(resolution: ${ratio}dppx)`).addEventListener("change", onChange, { once: true })
}

function updateResolutions() {
    for (const viewer of viewers.values()) {
        const rootResolution = getElementResolution(viewer.root)
        if (!resolutionEqual(rootResolution, viewer.rootResolution)) {
            viewer.rootResolution = rootResolution
            markDirty(viewer)
        }
    }
}

const resizeObserver = new ResizeObserver(updateResolutions)

listenToPixelRatio(() => {
    for (const viewer of viewers.values()) {
        markDirty(viewer)
    }
})

export function renderViewer(id: string, root: HTMLElement, desc: SceneDesc) {
    let viewer = viewers.get(id)
    const rootResolution = getElementResolution(root)
    if (!viewer) {
        viewer = {
            id, root, desc, rootResolution,
            state: { state: "empty" },
            dirty: false,
            lastRenderTime: -1000.0,
            prevRenderTime: -1000.0,
        }
        viewers.set(id, viewer)
        resizeObserver.observe(viewer.root)
        markDirty(viewer)
    }

    loadScene(desc.sceneName)

    const state = viewer.state
    if (viewer.root !== root) {
        switch (state.state) {
        case "canvas":
            root.appendChild(state.canvas)
            break;
        case "image":
            root.appendChild(state.image)
            break;
        case "realtime":
            root.appendChild(state.realtimeCanvas)
            break;
        }
        resizeObserver.unobserve(viewer.root)
        resizeObserver.observe(root)
        viewer.root = root
        markDirty(viewer)
    }

    if (!resolutionEqual(rootResolution, viewer.rootResolution)) {
        viewer.rootResolution = rootResolution
        markDirty(viewer)
    }

    if (!deepEqual(viewer.desc, desc)) {
        viewer.desc = desc
        markDirty(viewer)
    }

    const time = getTime()

    if (time - prevInteractedViewerTime < 0.5 && prevInteractedViewerId === id) {
        interactedViewerId = id
        interactedViewerTime = time
    }

    if (time - desc.latestInteractionTime < 0.5) {
        prevInteractedViewerId = id
        prevInteractedViewerTime = time
    }
}

export function removeViewer(id: string) {
    // TODO TODO
}

export function queryResolution(id: string) {
    const viewer = viewers.get(id)
    if (viewer) {
        return viewer.rootResolution
    } else {
        return null
    }
}

export function addSceneInfoListener(cb: SceneInfoListener) {
    sceneInfoListeners.push(cb)
}

function initializeNativeViewer() {
    window.setTimeout(() => {
        getRealtimeCanvas()
        rpcInitialized = true
        requestFrame()
    }, 0)
}

export function setupViewers() {
    rpcOnReady(initializeNativeViewer)
}

export function debugDumpViewers() {
    return Array.from(viewers.values()).map((viewer) => {
        return {
            id: viewer.id,
            state: viewer.state.state,
        }
    })
}
