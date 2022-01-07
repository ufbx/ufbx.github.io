import { h, Fragment } from "../../../ext/kaiku/dist/kaiku.dev"
import FbxViewer from "./fbx-viewer"
import Outliner from "./outliner"
import PropertySheet from "./property-sheet"

export default function DocViewer({ id }) {
    return (
        <div>
            <div style={{width:"50vw"}}>
            <div className="sp-top">
                <div className="sp-pane sp-outliner">
                    <Outliner id={ id } />
                    <PropertySheet id={ id } />
                </div>
                <div className="sp-pane sp-viewer">
                    <FbxViewer id={ id } />
                </div>
            </div>
            </div>
        </div>
    )
}
