import { h, Fragment, useRef, useEffect, unwrap } from  "../../../ext/kaiku/dist/kaiku.dev"
import { renderViewer, removeViewer } from "../viewer/viewer"
import globalState from "./global-state"

export default function FbxViewer({ id }) {
    const ref = useRef()
    const state = globalState.scenes[id]
    console.log("STATE", state)

    const mouseMove = (e) => {
        state.camera.position = [e.clientX*0.1, e.clientY*0.1, 10]
    }

    useEffect(() => removeViewer(id))

    useEffect(() => {
        const cur = ref.current
        console.log("rendering?!")
        if (cur) renderViewer(id, unwrap(cur), state)
    })

    return <div
        class="viewer ufbx-canvas-container"
        ref={ref}
        onMouseMove={mouseMove}
    ></div>
}