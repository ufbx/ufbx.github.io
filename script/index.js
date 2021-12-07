import { renderViewer, setupViewers } from "./viewer/viewer"

setupViewers()

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
}, 60)

window.setInterval(() => {
    const time = window.performance.now() / 1000.0
    renderViewer("b", document.getElementById("b"), {
        sceneName: "/static/models/barbarian.fbx",
        camera: {
            position: [20 * Math.sin(-time), 10, 20 * Math.cos(-time)],
            target: [0, 3, 0],
            fieldOfView: 10,
        },
    })
}, 60)

window.setInterval(() => {
    const time = window.performance.now() / 1000.0
    renderViewer("c", document.getElementById("c"), {
        sceneName: "/static/models/barbarian.fbx",
        camera: {
            position: [20 * Math.sin(time*3.0), 10, 20 * Math.cos(time*2.0)],
            target: [0, 3, 0],
            fieldOfView: 60,
        },
    })
}, 60)

window.setInterval(() => {
    const time = window.performance.now() / 1000.0
    renderViewer("d", document.getElementById("d"), {
        sceneName: "/static/models/barbarian.fbx",
        camera: {
            position: [20 * Math.sin(time*0.3), 40, 20 * Math.cos(time*0.3)],
            target: [0, 3, 0],
            fieldOfView: 5,
        },
    })
}, 60)


