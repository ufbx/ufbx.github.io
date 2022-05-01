
let hasCallbacks = false

let currentDrag = null
let dragReceiver = null

export function beginDrag(buttons, callback, state=null, userOpts=null) {
    if (!dragReceiver) {
        dragReceiver = document.createElement("div")
        dragReceiver.style.width = "100%"
        dragReceiver.style.height = "100%"
        dragReceiver.style.position = "absolute"
        dragReceiver.style.top = "0px"
        dragReceiver.style.left = "0px"
        dragReceiver.style.zIndex = "10"
        dragReceiver.style.opacity = "0%"

        dragReceiver.addEventListener("wheel", (e) => {
            if (!currentDrag) return
            if (!currentDrag.wheelCallback) return
            const buttons = currentDrag.buttons & e.buttons
            currentDrag.buttons = buttons
            if (buttons) {
                if (currentDrag.wheelCallback(e, currentDrag.state)) {
                    e.preventDefault()
                }
            }
        })

        dragReceiver.addEventListener("mousemove", (e) => {
            if (!currentDrag) return
            const buttons = currentDrag.buttons & e.buttons
            currentDrag.buttons = buttons
            if (buttons) {
                if (currentDrag.callback(e, currentDrag.state)) {
                    e.preventDefault()
                }
            } else {
                resetDrag()
            }
        })

        dragReceiver.addEventListener("mouseup", (e) => {
            if (!currentDrag) return
            const buttons = buttonToButtons(e.button)
            if (buttons & currentDrag.buttons) {
                e.preventDefault()
            }
            currentDrag.buttons = currentDrag.buttons & ~buttons
            if (!currentDrag.buttons) {
                const mouseUp = currentDrag.opts.mouseUp
                if (mouseUp) mouseUp(e, state)
                resetDrag()
            }
        })

        dragReceiver.addEventListener("blur", resetDrag)
    }

    const opts = userOpts ?? { }
    dragReceiver.style.cursor = opts.cursor || "inherit"

    if (!currentDrag) {
        const wheelCallback = opts.wheelCallback ?? null
        currentDrag = { callback, opts, state, buttons, wheelCallback }
        document.body.appendChild(dragReceiver)
    }
}

export function resetDrag() {
    if (currentDrag) {
        currentDrag = null
        document.body.removeChild(dragReceiver)
    }
}

export function buttonToButtons(button) {
    if (button === 0) {
        return 0x1
    } else if (button === 1) {
        return 0x4
    } else if (button === 2) {
        return 0x2
    } else {
        return 0
    }
}
