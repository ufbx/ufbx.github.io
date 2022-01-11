import { debugDumpViewers, renderViewer, setupViewers } from "./viewer/viewer"
import globalState from "./components/global-state"
import { h, Fragment, useState, useEffect, render, createState } from "../../ext/kaiku/dist/kaiku.dev"
import FbxViewer from "./components/fbx-viewer"
import Outliner from "./components/outliner"
import PropertySheet from "./components/property-sheet"
import DocViewer from "./components/doc-viewer"

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
  fieldOverrides: { },
  propOverrides: { },
  latestInteractionTime: -10000.0,
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
    globalState.scenes[id] = patchDefaults(desc, viewerDescDefaults)
    render(<DocViewer id={id} />, root, globalState)
}

const debugState = createState({ viewers: [] })

function ViewerDebug()
{
    const viewers = debugState.viewers
    return <div>
        {viewers.map(v => <div className={`vd-any vd-${v.state}`}>{v.id} {v.state}</div>)}
    </div>
}

window.setInterval(() => {
    debugState.viewers = debugDumpViewers()
}, 100)

const style = document.createElement("style")
style.innerText = `
.vd-any { width: 15em; }
.vd-empty { background-color: #888; }
.vd-canvas { background-color: #99f; }
.vd-realtime { background-color: #f99; }
.vd-image { background-color: #9f9; }
`
document.head.appendChild(style)

const debug = document.createElement("div")
document.querySelector("nav").appendChild(debug)

render(<ViewerDebug />, debug, debugState)

/*
window.setInterval(() => {
    const time = window.performance.now() / 1000.0
    renderViewer("a", document.getElementById("a"), {
        sceneName: "/static/models/barbarian.fbx",
        camera: {
            position: [20 * Math.sin(time), 10, 20 * Math.cos(time)],
            target: [0, 3, 0],
            fieldOfView: 30,
        },
    })
}, 16)
*/

/*

window.setInterval(() => {
    const time = window.performance.now() / 1000.0
    renderViewer("a", document.getElementById("a"), {
        sceneName: "/static/models/barbarian.fbx",
        camera: {
            position: [20 * Math.sin(-time), 10, 20 * Math.cos(-time)],
            target: [0, 3, 0],
            fieldOfView: 10,
        },
    }, { interactive: Math.random() < 0.2 })
}, 50000)

document.getElementById("b").addEventListener("mousemove", e => {
    if (e.buttons & 1) {
        renderViewer("b", document.getElementById("b"), {
            sceneName: "/static/models/barbarian.fbx",
            camera: {
                position: [e.clientX*-0.1+40, e.clientY*0.1, 20],
                target: [0, 3, 0],
                fieldOfView: 60,
            },
        }, { interactive: true })
        e.preventDefault()
    }
})

document.getElementById("c").addEventListener("mousemove", e => {
    if (e.buttons & 1) {
        renderViewer("c", document.getElementById("c"), {
            sceneName: "/static/models/barbarian.fbx",
            camera: {
                position: [e.clientX*-0.1+40, e.clientY*0.1, 20],
                target: [0, 3, 0],
                fieldOfView: 60,
            },
        }, { interactive: true })
        e.preventDefault()
    }
})

*/

/*
window.setInterval(() => {
    const time = window.performance.now() / 1000.0
    renderViewer("d", document.getElementById("d"), {
        sceneName: "/static/models/barbarian.fbx",
        camera: {
            position: [20 * Math.sin(time*0.3), 40, 20 * Math.cos(time*0.3)],
            target: [0, 3, 0],
            fieldOfView: 5,
        },
    }, { interactive: Math.random() < 0.05 })
}, 100)
*/

