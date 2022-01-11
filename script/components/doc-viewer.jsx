import { h, Fragment } from "kaiku"
import FbxViewer from "./fbx-viewer"
import Outliner from "./outliner"
import PropertySheet from "./property-sheet"

export default function DocViewer({ id }) {
    const state = globalState.scenes[id]
    return (
        <div className="sp-top">
            <div className="sp-pane sp-outliner">
                <Outliner id={ id } />
                {state.props.show ? <PropertySheet id={ id } /> : null}
            </div>
            <div className="sp-pane sp-viewer">
                <FbxViewer id={ id } />
            </div>
        </div>
    )
}
