import { h, Fragment, useEffect } from "kaiku"
import FbxViewer from "./fbx-viewer"
import { addBlobFile } from "../viewer/viewer"
import Outliner from "./outliner"
import PropertySheet from "./property-sheet"
import LoadOptions from "./load-options"
import globalState from "./global-state"
import Icon from "./icon"
import { IconMaximize, IconMinimize, IconUpload, IconReset } from "../icons"
import { deepUnwrap } from "../common"

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
        state.selectedNode = -1
    };

    const reset = () => {
        const original = globalState.originalScenes[id]
        for (const key of Object.keys(original)) {
            state[key] = deepUnwrap(original[key])
        }
    }

    const expand = () => {
        globalState.expandedId = globalState.expandedId === id ? null : id
    };

    const expanded = globalState.expandedId === id

    console.log(state.options)

    return (
        <div className="dv-top">
            <div className="dv-menu">
                <button key="reset" className="dv-button" onClick={reset}>
                    <Icon svg={IconReset} />
                    Reset
                </button>
                <label className="dv-button">
                    <input className="dv-file-input" type="file" onChange={uploadFile} />
                    <Icon svg={IconUpload} />
                    Select file
                </label>
                {expanded ?
                    <button key="expanded" className="dv-button" onClick={expand}>
                        <Icon svg={IconMinimize} />
                        Close
                    </button> :
                    <button key="inline" className="dv-button" onClick={expand}>
                        <Icon svg={IconMaximize} />
                        Expand
                    </button>}
            </div>
            <div className="sp-top dv-content">
                <div className="sp-pane sp-outliner">
                    <Outliner id={ id } />
                    {state.props.show ? <PropertySheet id={ id } /> : null}
                    {state.options.show ? <LoadOptions id={ id } /> : null}
                </div>
                <div className="sp-pane sp-viewer">
                    <FbxViewer id={ id } allowRawScroll={ expanded } />
                </div>
            </div>
        </div>
    )
}
