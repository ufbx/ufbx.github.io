import { h, Fragment, Component, useState, useRef, useEffect } from "kaiku"

export default function VirtualList({ rowFn, rowCount, className={}, overflowCount=5 }) {
    const state = useState({
        beginIndex: 0,
        endIndex: 0,
    })

    const parentRef = useRef(null)

    function update() {
        const parent = parentRef.current
        if (parent) {
            const top = parent.scrollTop
            const { height } = parent.getBoundingClientRect()
            const bottom = top + height
            state.beginIndex = Math.floor(top / 32)
            state.endIndex = Math.ceil(bottom / 32)
        }
    }

    useEffect(update)

    const tableHeight = `${rowCount * 32}px`

    const beginIndex = Math.min(Math.max(state.beginIndex - overflowCount, 0), rowCount)
    const endIndex = Math.min(Math.max(state.endIndex + overflowCount, 0), rowCount)

    return <div
            className={className}
            ref={parentRef}
            style={{
                overflowY: "scroll",
            }}
            onScroll={update}>
        <div style={{ height: tableHeight, position: "relative" }}>
            {new Array(endIndex - beginIndex).fill().map((_, i) => {
                const index = beginIndex + i
                const top = `${index * 32}px`
                return <div style={{
                    position: "absolute",
                    width: "100%",
                    height: "32px",
                    top,
                    }}>
                    {rowFn(index)}
                </div>
            })}
        </div>
    </div>
}
