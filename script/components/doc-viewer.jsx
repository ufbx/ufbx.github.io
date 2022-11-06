import { h, Fragment, useEffect } from "kaiku"
import FbxViewer from "./fbx-viewer"
import { addBlobFile } from "../viewer/viewer"
import Outliner from "./outliner"
import PropertySheet from "./property-sheet"
import globalState from "./global-state"

function closeExpandedDocViewer(event) {
    if (!event.target.hasAttribute("data-dv-popout")) return
    globalState.expandedId = null
}

window.addEventListener("keydown", (event) => {
    if (event.code === "Escape") {
        if (globalState.expandedId) {
            globalState.expandedId = null
            event.stopPropagation()
        }
    }
})

let prevExpandedId = null
useEffect(() => {
    const expandedId = globalState.expandedId
    if (expandedId === prevExpandedId) return

    if (prevExpandedId !== null) {
        const element = document.getElementById(`container-${prevExpandedId}`)
        element.classList.add("dv-inline")
        element.classList.remove("dv-expanded")
        element.removeEventListener("click", closeExpandedDocViewer, true)
    }

    if (expandedId !== null) {
        const element = document.getElementById(`container-${expandedId}`)
        element.classList.remove("dv-inline")
        element.classList.add("dv-expanded")
        element.addEventListener("click", closeExpandedDocViewer, true)
    }

    prevExpandedId = expandedId
})

export default function DocViewer({ id }) {
    const state = globalState.scenes[id]

    const uploadFile = (event) => {
        const input = event.target
        if (input.files.length === 0) return

        const file = input.files[0]
        const name = `File ${file.name}`

        addBlobFile(name, file)
        state.scene = name
        state.selectedElement = -1
    };

    const expand = () => {
        globalState.expandedId = globalState.expandedId === id ? null : id
    };

    const expanded = globalState.expandedId === id

    return (
        <div className="dv-top">
            <div className="dv-menu">
                <input className="dv-file-input" type="file" onChange={uploadFile} />
                <button className="dv-button" onClick={expand}>{expanded ? "Close" : "Expand"}</button>
            </div>
            <div className="sp-top dv-content">
                <div className="sp-pane sp-outliner">
                    <Outliner id={ id } />
                    {state.props.show ? <PropertySheet id={ id } /> : null}
                </div>
                <div className="sp-pane sp-viewer">
                    <FbxViewer id={ id } allowRawScroll={ expanded } />
                </div>
            </div>
        </div>
    )
}
