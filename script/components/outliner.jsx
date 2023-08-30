import globalState from "./global-state"
import { h, Fragment } from "kaiku"
import { elementTypeCategory, typeToIconUrl } from "./common"

function TreeNode({ state, info, id, level=0 }) {
    const element = info.elements[id]
    const icon = typeToIconUrl(element.type)
    const paddingRem = 0.25+level
    const padding = `${paddingRem}rem`
    let attribIcon = null
    let attribStruct = null
    let attribName = null
    let attrib = null

    let children = []
    if (element.type === "node") {
        if (state.outliner.showAttributes) {
            children.push(...element.attribs)
        } else {
            if (element.attribType && !element.isRoot && element.attribs.length > 0) {
                attribIcon = typeToIconUrl(element.attribType)
                attribStruct = `ufbx_${element.attribType}`
                attrib = info.elements[element.attribs[0]]
                attribName = attrib.name || `(${attrib.type} ${attrib.typedId})`
            }
        }
        children.push(...element.children)
    } else if (element.type === "mesh") {
        if (state.outliner.showMaterials) children.push(...element.materials)
        if (state.outliner.showDeformers) children.push(...element.deformers)
    } else if (element.type === "material") {
        children = element.textures
    } else if (element.type === "skin_deformer") {
        children = element.clusters
    } else if (element.type === "blend_deformer") {
        children = element.channels
    } else if (element.type === "blend_channel") {
        children = element.keyframes
    }

    const onClick = (e) => {
        e.preventDefault()
        e.stopPropagation()
        state.selectedNode = id
        state.selectedElement = id
    }
    const onClickAttrib = (e) => {
        e.preventDefault()
        e.stopPropagation()
        state.selectedNode = id
        state.selectedElement = attrib.id
    }
    const onKeyPress = (e) => {
        if (e.code === "Space" || e.code === "Enter") {
            onClick()
        }
    }

    const category = elementTypeCategory[element.type]
    const catClass = `cat-${category}`
    const structName = `ufbx_${element.type}`

    let name = element.name
    if (!name) {
        if (element.type === "node" && element.isRoot) {
            name = "(root)"
        }
    }

    const selected = state.selectedNode === id
    const nodeSelected = state.selectedElement === id
    const attribSelected = state.selectedElement === attrib?.id
    return <li className="ol-node">
        <div
            className={() => ({
                "ol-row": true,
                "ol-selected": selected,
                "ol-node-selected": nodeSelected,
                [catClass]: true,
            })}
            role="button"
            aria-label={`${element.type} ${name}`}
            tabIndex="0"
            style={{paddingLeft: padding}}
            onKeyPress={onKeyPress}
            >
            <div 
                className="ol-name"
                onClick={onClick}
            >
                <img className="ol-icon" src={icon} title={structName} alt="" aria-hidden="true" />
                <span>{name}</span>
            </div>
            {element.type === "node" && attribIcon ?
                <div    
                    className={{
                        "ol-attrib": true,
                        "ol-attrib-selected": attribSelected,
                    }}
                    onClick={onClickAttrib}
                >
                    <img className="ol-icon" src={attribIcon} title={attribStruct} alt="" aria-hidden="true" />
                    {selected ? <span>{attribName}</span> : null}
                </div>
            : null}
            {/*<span className="ol-type">{element.type}</span>*/}
        </div>
        {children.length > 0 ?
            <ul className="ol-list ol-nested">
                <div className="ol-ruler" style={{
                    left: `${paddingRem+0.6}rem`,
                }}>
                </div>
                {children.map(c => <TreeNode state={state} info={info} id={c} level={level+1} />)}
            </ul>
        : null}
    </li>
}

export default function Outliner({ id }) {
    const state = globalState.scenes[id]
    const info = globalState.infos[state.scene]
    if (!info) return null
    const rootId = info.rootNode
    return <div class="ol-container">
            <div className="ol-top">
            <ul className="ol-list" role="tree">{
                state.outliner.includeRoot ? (
                    <TreeNode state={state} info={info} id={rootId} />
                ) : (
                    info.elements[rootId].children.map(c => 
                        <TreeNode state={state} info={info} id={c} />)
                )
            }</ul>
        </div>
    </div>
}

