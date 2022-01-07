import globalState from "./global-state"
import { h, Fragment } from "../../../ext/kaiku/dist/kaiku.dev"
import { elementTypeCategory, typeToIconUrl } from "./common"

function TreeNode({ state, info, id, level=0 }) {
    const element = info.elements[id]
    const icon = typeToIconUrl(element.type)
    const padding = `${level}em`

    let children = []
    if (element.type === "node") {
        children = [...element.attribs, ...element.children]
    } else if (element.type === "mesh") {
        children = [...element.materials, ...element.deformers]
    } else if (element.type === "material") {
        children = element.textures
    } else if (element.type === "skin_deformer") {
        children = element.clusters
    } else if (element.type === "blend_deformer") {
        children = element.channels
    } else if (element.type === "blend_channel") {
        children = element.keyframes
    }

    const onClick = () => {
        state.selectedElement = id
    }
    const onKeyPress = (e) => {
        if (e.code === "Space" || e.code === "Enter") {
            onClick()
        }
    }

    const selected = state.selectedElement === id

    const category = elementTypeCategory[element.type]
    const catClass = `cat-${category}`
    const structName = `ufbx_${element.type}`

    return <li className="ol-node">
        <div
            className={{
                "ol-row": true,
                "ol-selected": selected,
                [catClass]: true,
            }}
            role="button"
            aria-label={`${element.type} ${element.name}`}
            tabIndex="0"
            style={{paddingLeft: padding}}
            onClick={onClick}
            onKeyPress={onKeyPress}
            >
            <img className="ol-icon" src={icon} title={structName} alt="" aria-hidden="true" />
            <span>{element.name}</span>
            <span className="ol-type">{element.type}</span>
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
        <ul className="ol-list" role="tree">{
            state.outliner.includeRoot ? (
                <TreeNode state={state} info={info} id={rootId} />
            ) : (
                info.elements[rootId].children.map(c => 
                    <TreeNode state={state} info={info} id={c} />)
            )
        }</ul>
    </div>
}

