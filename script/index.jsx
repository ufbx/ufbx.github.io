import { renderViewer, setupViewers } from "./viewer/viewer"
import FbxViewer from "./components/fbx-viewer"
import globalState from "./components/global-state"
import { h, Fragment, useState, useEffect, render, createState } from "../../ext/kaiku/dist/kaiku.dev"

setupViewers()

const state = createState({
    time: 0,
})

export function Top() {
    const topState = useState({})

    useEffect(() => {
        const time = state.time
        const time2 = state.time2
        topState.desc = {
            sceneName: "/static/models/barbarian.fbx",
            camera: {
                position: [20 * Math.sin(time), 10, 20 * Math.cos(time)],
                target: [0, 3, 0],
                fieldOfView: 30,
            },
        }
    })

    return (
        <div>
            <FbxViewer id="barb" />
            <FbxViewer id="barb2" />
            <FbxViewer id="barb3" />
        </div>
    )
}

;(function loop() {
  window.requestAnimationFrame(loop)
    state.time = performance.now() / 1000
})();

render(<Top />, document.querySelector("#root"), globalState)

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

