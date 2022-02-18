import { h, Fragment, useState, useRef, useEffect } from "kaiku"
import { typeToIconUrl } from "./common"
import { beginDrag, buttonToButtons } from "./global-drag"
import { deepEqual, getTime } from "../common"

function getField(ctx, name) {
    const { state, info, elementId } = ctx
    const override = state.fieldOverrides[`${elementId}.${name}`]
    if (override) {
        return { value: override, override: true }
    }
    return { value: info.elements[elementId].fields[name], override: false }
}

function setField(ctx, name, value) {
    const { state, info, elementId } = ctx
    const key = `${elementId}.${name}`
    if (state.fieldOverrides.hasOwnProperty(key) || !deepEqual(value, info.elements[elementId].fields[name])) {
        state.fieldOverrides[key] = value
        state.latestInteractionTime = getTime()
    }
}

function resetField(ctx, name) {
    const { state, elementId } = ctx
    // HACK: `delete` does not trigger an update
    state.fieldOverrides[`${elementId}.${name}`] = undefined
    delete state.fieldOverrides[`${elementId}.${name}`]
}

function getProp(ctx, index) {
    const { state, info, elementId } = ctx
    const override = state.propOverrides[`${elementId}.${index}`]
    if (override) {
        return { value: override, override: true }
    }
    return { value: info.elements[elementId].props[index].value, override: false }
}

function setProp(ctx, index, value) {
    const { state, elementId } = ctx
    state.propOverrides[`${elementId}.${index}`] = value
}

function resetProp(ctx, index) {
    const { state, elementId } = ctx
    // HACK: `delete` does not trigger an update
    state.propOverrides[`${elementId}.${index}`] = undefined
    delete state.propOverrides[`${elementId}.${index}`]
}

function maybeRound(flag, value) {
    return flag ? Math.round(value) : value
}

function NumberInput({ label, value, onInput, spec }) {
    const state = useState({
        useInput: false,
        inputValue: "",
        inputRef: { current: null },
        currentDrag: false,
        currentTime: 0,
        prevDrag: false,
        prevTime: 0,
    })

    useEffect(() => {
        const input = state.inputRef.current
        if (input) {
            input.select()
        }
    })

    const scale = spec.scale ?? 1.0
    const min = spec.min ?? -Infinity
    const max = spec.max ?? +Infinity
    const integer = spec.integer ?? false

    if (state.useInput) {
        return <input
            ref={state.inputRef}
            className="ps-input ps-typing"
            value={state.inputValue}
            aria-label={label}
            onKeyDown={e => {
                const code = e.code
                if (code == "Escape") {
                    state.useInput = false
                } else if (code == "Enter" || code == "NumpadEnter") {
                    const num = parseFloat(e.target.value)
                    if (!isNaN(num)) {
                        onInput(maybeRound(integer, num))
                    }
                    state.useInput = false
                }
            }}
            onBlur={e => {
                if (e.relatedTarget && e.relatedTarget.classList.contains("ps-input")) {
                    const num = parseFloat(e.target.value)
                    if (!isNaN(num)) {
                        onInput(maybeRound(integer, num))
                    }
                }
                state.useInput = false
            }}
        />
    } else {
        const precision = spec.integer ? 0 : Math.min(Math.max(4 - Math.ceil(Math.log10(Math.abs(value))), 0), 2)
        const str = value.toFixed(precision)
        return <div
            className="ps-input ps-draggable"
            tabIndex="0"
            onMouseDown={e => {
                if (e.ctrlKey || e.shiftKey) return
                const buttons = buttonToButtons(e.button)
                if (buttons & 0x1) {
                    let dragActivated = false
                    const startTime = Date.now()
                    const x = e.clientX
                    state.currentTime = startTime
                    state.currentDrag = false
                    beginDrag(buttons, (de) => {
                        const dx = (de.clientX - x) * 0.01 * scale
                        const curTime = Date.now()
                        if (Math.abs(de.clientX - x) > 4 || (curTime - startTime) > 100) {
                            dragActivated = true
                            state.currentDrag = true
                        }
                        if (dragActivated) {
                            onInput(maybeRound(integer, Math.min(Math.max(value + dx, min), max)))
                        }
                    }, null, {
                        cursor: "ew-resize",
                        mouseUp: (de) => {
                            const curTime = Date.now()
                            if ((curTime - state.prevTime) < 300 && !state.prevDrag && !state.currentDrag) {
                                state.useInput = true
                                state.inputValue = parseFloat(str) === value ? str : value.toString()
                            }

                            state.prevTime = state.currentTime
                            state.prevDrag = state.currentDrag
                        },
                    })
                    e.preventDefault()
                }
            }}
            onClick={e => {
                if (e.ctrlKey || e.shiftKey) {
                    state.useInput = true
                    state.inputValue = parseFloat(str) === value ? str : value.toString()
                }
            }}
            onFocus={e => {
                state.useInput = true
                state.inputValue = parseFloat(str) === value ? str : value.toString()
            }}
        >{str}</div>
    }
}

function OverrideIndicator({ override, onClick }) {
    return <button
        className={{
            "ps-left": true,
            "ps-override": override,
        }}
        onClick={onClick}
        tabIndex={override ? 0 : -1}
        title="Reset to default"
        enabled={override}
    />
}

function Label({ name, text }) {
    return <div className="ps-field">
        <div className="ps-name">{name}</div>
        <div>{text}</div>
    </div>
}

function FieldVec2({ ctx, name, label, spec }) {
    const { value, override } = getField(ctx, name)
    const { x, y } = value

    return <div className="ps-field" aria-label={name}>
        <OverrideIndicator override={override} onClick={() => resetField(ctx, name)} />
        <div className="ps-name">{label}</div>
        <NumberInput label={`${name} X`} spec={spec} value={x} onInput={v => setField(ctx, name, { x:v, y }) } />
        <NumberInput label={`${name} Y`} spec={spec} value={y} onInput={v => setField(ctx, name, { x, y:v }) } />
    </div>
}

function FieldVec3({ ctx, name, label, spec }) {
    const { value, override } = getField(ctx, name)
    const { x, y, z } = value

    return <div className="ps-field" aria-label={name}>
        <OverrideIndicator override={override} onClick={() => resetField(ctx, name)} />
        <div className="ps-name">{label}</div>
        <NumberInput label={`${name} X`} spec={spec} value={x} onInput={v => setField(ctx, name, { x:v, y, z }) } />
        <NumberInput label={`${name} Y`} spec={spec} value={y} onInput={v => setField(ctx, name, { x, y:v, z }) } />
        <NumberInput label={`${name} Z`} spec={spec} value={z} onInput={v => setField(ctx, name, { x, y, z:v }) } />
    </div>
}

function FieldColor({ ctx, name, spec }) {
    const { value, override } = getField(ctx, name)
    const { x, y, z } = value

    // TODO: Linear to sRGB?
    const cssR = Math.min(Math.max(x * 256, 0), 255) | 0
    const cssG = Math.min(Math.max(y * 256, 0), 255) | 0
    const cssB = Math.min(Math.max(z * 256, 0), 255) | 0
    const cssColor = `rgb(${cssR}, ${cssG}, ${cssB})`

    return <div className="ps-field" aria-label={name}>
        <OverrideIndicator override={override} onClick={() => resetField(ctx, name)} />
        <div className="ps-name">{name}</div>
        <div className="ps-color-square" style={{
            backgroundColor: cssColor,
        }} />
        <NumberInput label={`${name} red`} spec={spec} value={x} onInput={v => setField(ctx, name, { x:v, y, z }) } />
        <NumberInput label={`${name} green`} spec={spec} value={y} onInput={v => setField(ctx, name, { x, y:v, z }) } />
        <NumberInput label={`${name} blue`} spec={spec} value={z} onInput={v => setField(ctx, name, { x, y, z:v }) } />
    </div>
}

function FieldNumber({ ctx, name, spec }) {
    const { value, override } = getField(ctx, name)

    return <div className="ps-field" aria-label={name}>
        <OverrideIndicator override={override} onClick={() => resetField(ctx, name)} />
        <div className="ps-name">{name}</div>
        <NumberInput label={`${name}`} spec={spec} value={value} onInput={v => setField(ctx, name, v) } />
    </div>
}

function FieldGroup({ name, children }) {
    return <div role="group" aria-label={name}>
        <div className="ps-group">{name}</div>
        {children}
    </div>
}

function replaceAt(arr, index, value) {
    const copy = arr.slice()
    copy[index] = value
    return copy
}

function FieldProp({ ctx, index, prop }) {
    const { value, override } = getProp(ctx, index)
    const label = prop.name
    const spec = { }

    if (prop.type === "translation" || prop.type === "rotation" || prop.type === "scale" || prop.type === "vector") {
        return <div className="ps-field">
            <OverrideIndicator override={override} onClick={() => resetProp(ctx, index)} />
            <div className="ps-name">{label}</div>
            <NumberInput spec={spec} value={value[0]} onInput={v => setProp(ctx, index, replaceAt(value, 0, v)) } />
            <NumberInput spec={spec} value={value[1]} onInput={v => setProp(ctx, index, replaceAt(value, 1, v)) } />
            <NumberInput spec={spec} value={value[2]} onInput={v => setProp(ctx, index, replaceAt(value, 2, v)) } />
        </div>
    } else if (prop.type === "number") {
        return <div className="ps-field">
            <OverrideIndicator override={override} onClick={() => resetProp(ctx, index)} />
            <div className="ps-name">{label}</div>
            <NumberInput spec={spec} value={value[0]} onInput={v => setProp(ctx, index, replaceAt(value, 0, v)) } />
        </div>
    } else if (prop.type === "integer") {
        spec.integer = true
        return <div className="ps-field">
            <OverrideIndicator override={override} onClick={() => resetProp(ctx, index)} />
            <div className="ps-name">{label}</div>
            <NumberInput spec={spec} value={value[0]} onInput={v => setProp(ctx, index, replaceAt(value, 0, v)) } />
        </div>
    } else {
    }
}

function NodeSheet({ ctx }) {
    return <>
        <FieldGroup name="local_transform">
            <FieldVec3 ctx={ctx} name="translation" label="translation" spec={{ scale: 1.0 }} />
            <FieldVec3 ctx={ctx} name="rotation" label="rotation" spec={{ scale: 60.0 }} />
            <FieldVec3 ctx={ctx} name="scale" label="scale" spec={{ scale: 1.0 }}/>
        </FieldGroup>
        {ctx.state.props.showGeometricTransform ? (
            <FieldGroup name="geometry_transform">
                <FieldVec3 ctx={ctx} name="geo_translation" label="translation" spec={{ scale: 1.0 }} />
                <FieldVec3 ctx={ctx} name="geo_rotation" label="rotation" spec={{ scale: 60.0 }} />
                <FieldVec3 ctx={ctx} name="geo_scale" label="scale" spec={{ scale: 1.0 }}/>
            </FieldGroup>
        ) : null}
    </>
}

function LightSheet({ ctx }) {
    return <>
        <FieldColor ctx={ctx} name="color" spec={{ scale: 0.5, min: 0.0, max: 1.0 }} />
        <FieldNumber ctx={ctx} name="intensity" spec={{ scale: 1.0, min: 0.0 }} />
    </>
}

function CameraSheet({ ctx }) {
    return <>
        <FieldVec2 ctx={ctx} name="resolution" label="resolution" spec={{ scale: 1.0 }} />
        <FieldVec2 ctx={ctx} name="field_of_view_deg" label="field_of_view_deg" spec={{ scale: 1.0 }} />
    </>
}

function MeshSheet({ ctx }) {
    const { info, elementId } = ctx
    const elem = info.elements[elementId]
    return <>
        <Label name="faces" text={elem.numFaces} />
        <Label name="vertices" text={elem.numVertices} />
    </>
}


function NullSheet({ ctx }) {
    return <>
    </>
}

const sheetByType = {
    node: NodeSheet,
    light: LightSheet,
    camera: CameraSheet,
    mesh: MeshSheet,
}

export default function PropertySheet({ id }) {
    const state = globalState.scenes[id]
    const info = globalState.infos[state.scene]
    if (!info) return null
    if (state.selectedElement < 0) return null

    const elementId = state.selectedElement
    const element = info.elements[elementId]
    const Sheet = sheetByType[element.type] ?? NullSheet

    const ctx = {
        state, info, elementId,
    }

    const structName = `ufbx_${element.type}`
    const icon = typeToIconUrl(element.type)

    return <div className="ps-top" aria-label={`${element.name} properties`}>
        <div className="ps-titlearea">
            <div className="ps-title">
                <img className="ps-icon" src={icon} title={structName} alt={element.type} />
                <span>{element.name}</span>
            </div>
            <div className="ps-subtitle">
                <span>{structName}</span>
            </div>
        </div>
        <div className="ps-sheet">
            <Sheet ctx={ctx} />
            {/*{element.props.map((p,ix) => <FieldProp ctx={ctx} prop={p} index={ix} />)}*/}
        </div>
    </div>
}
