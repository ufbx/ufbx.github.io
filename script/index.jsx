import { debugDumpViewers, renderViewer, setupViewers } from "./viewer/viewer"
import globalState from "./components/global-state"
import { h, Fragment, useState, useEffect, render, createState, unwrap } from "kaiku"
import FbxViewer from "./components/fbx-viewer"
import Outliner from "./components/outliner"
import PropertySheet from "./components/property-sheet"
import DocViewer from "./components/doc-viewer"
import VertexDisplay from "./components/vertex-display"
import { deepUnwrap } from "./common"

window.unwrap = unwrap

setupViewers()

;() => {
    function Top() {
        const topState = useState({})

        return (
            <div>
                {/*
            <div className="sp-top">
                <div className="sp-pane sp-outliner">
                    <Outliner id="barb" />
                </div>
                <div className="sp-pane sp-viewer">
                    <FbxViewer id="barb" />
                </div>
            </div>
            <div className="sp-top">
                <div className="sp-pane sp-outliner">
                    <Outliner id="barb2" />
                </div>
                <div className="sp-pane sp-viewer">
                    <FbxViewer id="barb2" />
                </div>
            </div>
            <div>
                <input type="range" min="0.0" max="2.8" step="0.01" style={{ width: "800px" }}
                    onChange={(e) => globalState.scenes.barb.animation.time = e.target.valueAsNumber}
                    onInput={(e) => globalState.scenes.barb.animation.time = e.target.valueAsNumber} />
            </div>
                */}

            <div style={{width:"50vw"}}>
            <div className="sp-top">
                <div className="sp-pane sp-outliner">
                    <Outliner id="defaultCube" />
                    <PropertySheet id="defaultCube" />
                </div>
                <div className="sp-pane sp-viewer">
                    <FbxViewer id="defaultCube" />
                </div>
            </div>

            </div>


            </div>
        )
    }
}

const viewerDescDefaults = {
  camera: {
    yaw: 0,
    pitch: 0,
    distance: 30,
    offset: { x: 0, y: 0, z: 0 },
  },
  outliner: {
    includeRoot: false,
    showAttributes: false,
    showMaterials: false,
    showDeformers: false,
  },
  props: {
    show: false,
    showGeometricTransform: false,
  },
  animation: {
    time: 0.0,
  },
  selectedElement: -1,
  selectedNode: -1,
  fieldOverrides: { },
  propOverrides: { },
  latestInteractionTime: -10000.0,
  highlightVertexIndex: -1,
  highlightFaceIndex: -1,
  widgetScale: 1,
}

function patchDefaults(dst, defaults) {
    if (dst === undefined) return defaults
    if (typeof dst === "object" && !Array.isArray(dst)) {
        for (const key in defaults) {
            dst[key] = patchDefaults(dst[key], defaults[key])
        }
    }
    return dst
}

for (const id in viewerDescs) {
    const desc = viewerDescs[id]
    const root = document.querySelector(`#root-${id}`)
    const scene = patchDefaults(desc, viewerDescDefaults)
    globalState.scenes[id] = deepUnwrap(scene)
    globalState.originalScenes[id] = deepUnwrap(scene)
    render(<DocViewer id={id} />, root)
}

for (const elem of document.querySelectorAll("[data-viewer-id]")) {
    const id = elem.dataset.viewerId
    render(<VertexDisplay id={id} />, elem)
}
