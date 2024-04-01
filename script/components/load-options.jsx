import { h, Fragment, useEffect } from "kaiku"

const specs = {
    geometryTransformHandling: {
        type: "enum",
        label: "geometry_transform_handling",
        options: [
            { name: "preserve", label: "PRESERVE" },
            { name: "helperNodes", label: "HELPER_NODES" },
            { name: "modifyGeometry", label: "MODIFY_GEOMETRY" },
        ],
    },
}

function EnumOption({ ctx, name }) {
    const spec = specs[name]
    const key = `opt-${ctx.id}`
    return <fieldset>
         <legend>{spec.label}</legend>
        {spec.options.map(opt => (
            <div>
                <input type="radio" name={name} value={opt.name} id={`${key}-${opt.name}`} />
                <label for={`${key}-${opt.name}`}>{opt.label}</label>
            </div>
        ))}
    </fieldset>
}

function LoadOption({ ctx, name }) {
    const spec = specs[name]
    if (spec.type === "enum") {
        return <EnumOption ctx={ctx} name={name} />
    } else {
        return null
    }
}

export default function LoadOptions({ id }) {
    const state = globalState.scenes[id]
    if (!state) return null
    const options = state.options
    if (!options) return null

    const ctx = {
        id, state, options,
    }

    return <div className="ps-top">
        <div className="ps-titlearea">
            <div className="ps-title">
                <span>Load options</span>
            </div>
        </div>
        <div className="ps-sheet">
            {options.visible.map(name => <LoadOption ctx={ctx} name={name} />)}
        </div>
    </div>
}

