let prevHoverId = null
let prevHoverDecls = []
let prevHoverRefs = []

let decls = { }
let refs = { }

function updateHover(id) {
    const prevId = prevHoverId
    if (prevHoverId !== null) {
        for (const elem of decls[prevId]) elem.classList.remove("c-hover")
        for (const elem of refs[prevId]) elem.classList.remove("c-hover")
    }

    for (const elem of decls[id]) elem.classList.add("c-hover")
    for (const elem of refs[id]) elem.classList.add("c-hover")

    prevHoverId = id
}

function endHover(id) {
    if (id === prevHoverId) {
        for (const elem of decls[id]) elem.classList.remove("c-hover")
        for (const elem of refs[id]) elem.classList.remove("c-hover")
        prevHoverId = null
    }
}

for (const elem of document.querySelectorAll("[data-ref-id]")) {
    const id = elem.dataset.refId
    if (!decls[id]) decls[id] = []
    if (!refs[id]) refs[id] = []
    refs[id].push(elem)
    elem.addEventListener("mouseover", () => updateHover(id))
    elem.addEventListener("mouseout", () => endHover(id))
}
for (const elem of document.querySelectorAll("[data-decl-id]")) {
    const id = elem.dataset.declId
    if (!decls[id]) decls[id] = []
    if (!refs[id]) refs[id] = []
    decls[id].push(elem)
    elem.addEventListener("mouseover", () => updateHover(id))
    elem.addEventListener("mouseout", () => endHover(id))
}