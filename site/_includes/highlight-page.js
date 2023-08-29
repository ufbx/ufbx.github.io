
function normalizePath(path) {
    return path.replace(/\/$/, "")
}

const elements = document.querySelectorAll("#g-left-nav a")
const path = normalizePath(window.location.pathname)
for (const elem of elements) {
    const href = elem.getAttribute("href")
    if (normalizePath(href) === path) {
        elem.classList.add("active")
    }
}
