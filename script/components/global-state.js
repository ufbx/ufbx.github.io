import { createState } from "../../../ext/kaiku/dist/kaiku.dev"

const globalState = createState({
    scenes: {
        barb: {
            scene: "/static/models/barbarian.fbx",
            camera: {
                yaw: 0,
                pitch: 0,
                distance: 30,
                offset: { x: 0, y: 0, z: 0 },
            },
            outliner: {
                includeRoot: false,
            }
        },
        barb2: {
            scene: "/static/models/barbarian.fbx",
            camera: {
                yaw: 0,
                pitch: 0,
                distance: 30,
                offset: { x: 0, y: 0, z: 0 },
            },
            outliner: {
                includeRoot: true,
            }
        },
    },
    infos: { },
})

window.globalState = globalState

export default globalState
