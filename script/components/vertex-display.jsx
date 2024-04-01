import { h, Fragment, useState, useEffect } from "kaiku"
import VirtualTable from "./virtual-table"
import { rpcCall } from "../viewer/rpc"

function vec3ToString(v) {
    const x = v.x.toFixed(2)
    const y = v.y.toFixed(2)
    const z = v.z.toFixed(2)
    return `(${x}, ${y}, ${z})`
}

function VertexRow({ viewerId, sceneName, elementId, index }) {
    const sceneState = globalState.scenes[viewerId]
    const state = useState({ row: null })
    useEffect(() => {
        state.row = rpcCall({
            cmd: "getVertex",
            sceneName, elementId, index,
        })
    })
    if (!state.row) return null
    const { vertexIndex, position, normal, uv, face } = state.row

    function onMouseOver() {
        sceneState.highlightVertexIndex = index
        sceneState.highlightFaceIndex = face
    }

    function onMouseOut() {
        if (sceneState.highlightVertexIndex === index) {
            sceneState.highlightVertexIndex = -1
            sceneState.highlightFaceIndex = -1
        }
    }

    return <div
            className={() => ["vw-row", { "vw-face-highlight": sceneState.highlightFaceIndex === face }]}
            onMouseOver={onMouseOver}
            onMouseOut={onMouseOut}
            >
        <div className="vw-index">{index}</div>
        <div className="vw-index">{vertexIndex}</div>
        <div className="vw-cell">{vec3ToString(position)}</div>
        <div className="vw-cell">{vec3ToString(normal)}</div>
    </div>
}

function VertexDisplayFallback() {
    return <div className="vw-top">
        <div className="vw-header"></div>
        <div className="vw-warning">
            Select a mesh to view vertices
        </div>
    </div>
}

export default function VertexDisplay({ id }) {
    const state = globalState.scenes[id]
    const info = globalState.infos[state.scene]
    if (!state || !info) return <VertexDisplayFallback />
    const { selectedElement } = state
    if (selectedElement < 0) return <VertexDisplayFallback />
    const element = info.elements[selectedElement]
    if (element.type !== "mesh") return <VertexDisplayFallback />
    const { numIndices } = element

    return <div className="vw-top">
        <div className="vw-header">
            <div className="vw-index">Index</div>
            <div className="vw-index">Vertex</div>
            <div className="vw-cell">Position</div>
            <div className="vw-cell">Normal</div>
        </div>
        <VirtualTable
            rowCount={numIndices}
            className="vw-list"
            rowFn={index => <VertexRow
                key={index}
                viewerId={id}
                sceneName={state.scene}
                elementId={selectedElement}
                index={index}
                />
            }
        />
    </div>
}
