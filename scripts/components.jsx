import { h, Fragment, render, createState, immutable } from 'kaiku'

const state = createState({
    scene: null,
    open: [],
    hoveredPath: "",
    hoveredElement: -1,
    selectedPath: "",
    selectedElement: -1,
    visualization: {
        hoveredElements: false,
    },
})

function UfbxElement({ element, path }) {
    const { id } = element
    const children = element.children ?? []
    const open = state.open[id]
    const hovered = path == state.hoveredPath
    const selected = path == state.selectedPath
    const elementHovered = id == state.hoveredElement
    const elementSelected = id == state.selectedElement
    return <li class="et-tree-node">
        <div
            className={{
                "et-element": true,
                "et-selected": selected,
                "et-selected-element": elementSelected && !selected,
                "et-hovered": hovered,
                "et-hovered-element": state.visualization.hoveredElements && elementHovered && !hovered,
            }}
            onClick={() => {
                state.selectedPath = path
                state.selectedElement = element.id
            } }
            onMouseEnter={() => {
                state.hoveredPath = path
                state.hoveredElement = element.id
            } }
            onMouseLeave={() => {
                if (state.hoveredPath == path) {
                    state.hoveredPath = ""
                    state.hoveredElement = -1
                }
            } }
        >
            <span
                class="et-arrow"
                onClick={ (e) => {
                    state.open[id] = !open
                    e.stopPropagation()
                } }
            >
                {children.length > 0 ? (open ? "\u25BC" : "\u25BA") : ""}
            </span>
            <img class="et-icon" src={`/icons/element/${element.type}.svg`} />
            <span class="et-space"></span>
            <span class="et-name">{ element.name }</span>
        </div>
        { children && open ? <ul class="et-tree">{ children.map((ix, i) =>
            <UfbxElement element={state.scene.elements[ix]} path={`${path}.${i}`} />
        ) }</ul> : null }
    </li>
}

function UfbxElementTree({ roots }) {
    return <ul class="et-tree">
        { roots.map((el, i) => <UfbxElement element={el} path={i.toString()} />) }
    </ul>
}

function UfbxProp({ prop }) {
    return <tr>
        <td>{prop.name}</td>
        <td>{prop.value}</td>
    </tr>
}

function UfbxPropList({ props }) {
    return <table className="pl-table">
        <tr>
            <th>Name</th>
            <th>Value</th>
        </tr>

        { props.map(p => <UfbxProp prop={p} />) }
    </table>
}

function UfbxModelViewer() {
    return <canvas>
        Temporary
    </canvas>
}

function UfbxTempRoot() {
    const scene = state.scene
    if (!scene) return null

    const elements = scene.elements
    const root = scene.root
    const selectedElement = state.selectedElement >= 0 ? elements[state.selectedElement] : null
    return <div className="tr-main">
        <div className="tr-left">
            <div className="tr-left-top">
                <UfbxElementTree roots={ [elements[root]] } />
            </div>
            <div className="tr-left-bottom">
                <UfbxPropList props={selectedElement?.props ?? []} />
            </div>
        </div>
        <div className="tr-right">
            <UfbxModelViewer />
        </div>
    </div>
}

export function setScene(scene) {
    state.scene = immutable(scene)
    state.open = Array(scene.elements.length).fill(false)
    state.open[0] = true
}

export function renderRoot(element) {
    render(<UfbxTempRoot />, element, state)
}