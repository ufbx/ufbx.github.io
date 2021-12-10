import { createState } from "../../../ext/kaiku/dist/kaiku.dev"

const globalState = createState({
    scenes: {
        barb: {
            sceneName: "/static/models/barbarian.fbx",
            camera: {
                position: [0, 3, 20],
                target: [0, 3, 0],
                fieldOfView: 30,
                nearPlane: 1.0,
                farPlane: 500,
            },
        },
        barb2: {
            sceneName: "/static/models/barbarian.fbx",
            camera: {
                position: [0, 3, 20],
                target: [0, 3, 0],
                fieldOfView: 10,
                nearPlane: 1.0,
                farPlane: 500,
            },
        },
        barb3: {
            sceneName: "/static/models/barbarian.fbx",
            camera: {
                position: [0, 20, 20],
                target: [0, 3, 0],
                fieldOfView: 30,
                nearPlane: 1.0,
                farPlane: 500,
            },
        },
    }
})

export default globalState
