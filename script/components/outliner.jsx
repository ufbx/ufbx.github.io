import globalState from "./global-state"
import { h, Fragment } from "../../../ext/kaiku/dist/kaiku.dev"

function TreeNode({ state, info, id, level=0 }) {
    const element = info.elements[id]
    const icon = `/static/icons/element/${element.type}.svg`
    const padding = `${level}em`

    let children = []
    if (element.type === "node") {
        children = [...element.attribs, ...element.children]
    } else if (element.type === "mesh") {
        children = element.materials
    } else if (element.type === "material") {
        children = element.textures
    }

    const onClick = () => {
        state.selectedElement = id
    }
    const selected = state.selectedElement === id

    return <li className="ol-node">
        <div
            className={{
                "ol-row": true,
                "ol-selected": selected,
            }}
            style={{paddingLeft: padding}}
            onClick={onClick}>
            <img className="ol-icon" src={icon} title={`ufbx_${element.type}`} alt={element.type} />
            <span>{element.name}</span>
        </div>
        <ul className="ol-list ol-nested">
            {children.map(c => <TreeNode state={state} info={info} id={c} level={level+1} />)}
        </ul>
    </li>
}

export default function Outliner({ id }) {
    const state = globalState.scenes[id]
    const info = globalState.infos[state.scene]
    if (!info) return null
    const rootId = info.rootNode
    return <div className="ol-top">
        <ul className="ol-list">{
            state.outliner.includeRoot ? (
                <TreeNode state={state} info={info} id={rootId} />
            ) : (
                info.elements[rootId].children.map(c => 
                    <TreeNode state={state} info={info} id={c} />)
            )
        }</ul>
    </div>
}

