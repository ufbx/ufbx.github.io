import { createState } from "../../../ext/kaiku/dist/kaiku.dev"

const globalState = createState({
    scenes: {
        defaultCube: {
            scene: "/static/models/blender_default_cube.fbx",
            camera: {
                yaw: 0,
                pitch: 0,
                distance: 30,
                offset: { x: 0, y: 0, z: 0 },
            },
            outliner: {
                includeRoot: false,
            },
            animation: {
                time: 0.0,
            },
            selectedElement: -1,
            fieldOverrides: { },
            propOverrides: { },
        },
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
            },
            animation: {
                time: 0.0,
            },
            selectedElement: -1,
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
            },
            animation: {
                time: 0.0,
            },
            selectedElement: -1,
        },
    },
    infos: { },
})

window.globalState = globalState

export default globalState
