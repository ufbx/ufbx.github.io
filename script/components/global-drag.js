
let hasCallbacks = false

let currentDrag = null
let dragReceiver = null

function initializeCallbacks() {
    /*
    window.addEventListener("mousemove", (e) => {
        if (!currentDrag) {
            if (!bodyHasMouseup) {
                document.body.classList.toggle("mouseup", true)
                bodyHasMouseup = true
            }
            return
        }
        if (bodyHasMouseup) {
            document.body.classList.toggle("mouseup", false)
            bodyHasMouseup = false
        }

        const buttons = currentDrag.buttons & e.buttons
        currentDrag.buttons = buttons
        if (buttons) {
            const dx = e.movementX
            const dy = e.movementY
            if (handleDrag(currentDrag.id, dx, dy, currentDrag.action)) {
                e.preventDefault()
            }
        } else {
            currentDrag = null
        }
    })
    window.addEventListener("mouseup", (e) => {
        if (!currentDrag) return
        const buttons = buttonToButtons(e.button)
        if (buttons & currentDrag.buttons) {
            e.preventDefault()
        }
        currentDrag.buttons = currentDrag.buttons & ~buttons
        if (!currentDrag.buttons) {
            currentDrag = null
            if (!bodyHasMouseup) {
                document.body.classList.toggle("mouseup", true)
                bodyHasMouseup = true
            }
        }
    })
    window.addEventListener("blur", resetDrag)
    */
}

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
                resetDrag()
            }
        })
        dragReceiver.addEventListener("blur", resetDrag)
    }

    const opts = userOpts ?? { }
    dragReceiver.style.cursor = opts.cursor || "inherit"

    if (!currentDrag) {
        currentDrag = { callback, state, buttons }
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
