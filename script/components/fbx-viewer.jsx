import { h, Fragment, useRef, useEffect, unwrap, immutable } from "kaiku"
import { getTime } from "../common"
import { mad3, cross3, normalize3, v3, add3 } from "../common/vec3"
import { renderViewer, removeViewer, queryResolution, addSceneInfoListener } from "../viewer/viewer"
import { beginDrag, buttonToButtons } from "./global-drag"
import globalState from "./global-state"

function yawPitch(yaw, pitch, radius=1) {
    const yc = Math.cos(yaw)
    const ys = Math.sin(yaw)
    const pc = Math.cos(pitch)
    const ps = Math.sin(pitch)
    return v3(ys*pc*radius, ps*radius, yc*pc*radius)
}

function stateToDesc(state) {
    const { camera, animation, fieldOverrides } = state
    const info = globalState.infos[state.scene]

    const yaw = camera.yaw * (Math.PI/180.0)
    const pitch = camera.pitch * (Math.PI/180.0)
    const radius = camera.distance

    const delta = yawPitch(yaw, pitch, radius)
    const target = camera.offset
    const position = add3(target, delta)

    const overrides = []

    for (const key in fieldOverrides) {
        const value = fieldOverrides[key]
        const [elementIdStr, name] = key.split(".", 2)
        const elementId = parseInt(elementIdStr)
        const type = info.elements[elementId].type
        if (type === "node") {
            if (name === "translation") {
                overrides.push({ elementId, name: "Lcl Translation", value: [value.x, value.y, value.z] })
            } else if (name === "rotation") {
                overrides.push({ elementId, name: "Lcl Rotation", value: [value.x, value.y, value.z] })
            } else if (name === "scale") {
                overrides.push({ elementId, name: "Lcl Scaling", value: [value.x, value.y, value.z] })
            } else if (name === "geo_translation") {
                overrides.push({ elementId, name: "GeometricTranslation", value: [value.x, value.y, value.z] })
            } else if (name === "geo_rotation") {
                overrides.push({ elementId, name: "GeometricRotation", value: [value.x, value.y, value.z] })
            } else if (name === "geo_scale") {
                overrides.push({ elementId, name: "GeometricScaling", value: [value.x, value.y, value.z] })
            }
        }
    }

    return {
        sceneName: state.scene,
        latestInteractionTime: state.latestInteractionTime,
        camera: {
            position,
            target,
            fieldOfView: 30,
            nearPlane: Math.min(Math.max(radius*0.1, 0.01), 1.0),
            farPlane: Math.max(radius*2.0, 100.0),
        },
        animation: {
            time: animation.time,
        },
        selectedElement: state.selectedElement,
        highlightVertexIndex: state.highlightVertexIndex,
        highlightFaceIndex: state.highlightFaceIndex,
        widgetScale: state.widgetScale,
        overrides,
    }
}

let currentDrag = null

function handleDrag(event, { id, action }) {
    const pixelDx = event.movementX
    const pixelDy = event.movementY

    const resolution = queryResolution(id)
    if (!resolution) return false
    const scale = 1.0 / Math.min(resolution.width, resolution.height)
    let dx = pixelDx * scale
    let dy = pixelDy * scale
    const state = globalState.scenes[id]

    if (action === "rotate") {
        state.camera.yaw -= dx * 90.0
        state.camera.pitch = Math.min(Math.max(state.camera.pitch + dy * 90.0, -80), 80)
        state.latestInteractionTime = getTime()
    } else if (action === "pan") {
        const { camera } = state
        const yaw = camera.yaw * (Math.PI/180.0)
        const pitch = camera.pitch * (Math.PI/180.0)
        const forward = yawPitch(yaw, pitch)
        const tempUp = v3(0, 1, 0)
        const right = normalize3(cross3(forward, tempUp))
        const up = normalize3(cross3(right, forward))
        const speed = camera.distance * 0.5
        camera.offset = mad3(mad3(camera.offset, right, dx * speed), up, dy * speed)
        state.latestInteractionTime = getTime()
    }

    return true
}

function getDragAction(buttons, shift) {
    if (shift) return "pan"
    if (buttons & 0x4) return "pan"
    return "rotate"
}

export default function FbxViewer({ id, allowRawScroll=false }) {
    const ref = useRef()
    const state = globalState.scenes[id]

    const rawWheel = (e) => {
        let scale = 0.1
        switch (e.deltaMode) {
            case WheelEvent.DOM_DELTA_PIXEL: scale = 0.04; break;
            case WheelEvent.DOM_DELTA_LINE:  scale = 1.33; break;
            case WheelEvent.DOM_DELTA_PAGE:  scale = 10.0; break; // FIXME: this is a guess
        }
        const distance = Math.min(Math.max(state.camera.distance * Math.pow(2.0, e.deltaY * scale * 0.03), 1.0), 1000.0)
        state.camera.distance = distance
        e.preventDefault()
    }

    const wheel = (e) => {
        if (!e.shiftKey && !allowRawScroll) return
        rawWheel(e)
    }

    const mouseDown = (e) => {
        if (currentDrag) return
        const buttons = buttonToButtons(e.button) & 0x5
        const action = getDragAction(buttons, e.shiftKey)
        if (buttons && action) {
            beginDrag(buttons, handleDrag, { id, action }, {
                wheelCallback: (e, _) => {
                    rawWheel(e)
                },
            })
            e.preventDefault()
        }
    }

    useEffect(() => () => removeViewer(id))

    useEffect(() => {
        const cur = ref.current
        const desc = stateToDesc(state)
        if (cur) renderViewer(id, unwrap(cur), desc)
    })

    return <div
        class="ufbx-viewer ufbx-canvas-container"
        ref={ref}
        onMouseDown={mouseDown}
        onWheel={wheel}
    ></div>
}

addSceneInfoListener((name, info) => {
    globalState.infos[name] = immutable(info)
})