
export function deepEqual(a, b) {
    if (a === b) return true
    const type = typeof a
    if (type !== typeof b) return false
    if (type === "number") {
        return Math.abs(a - b) <= 1e-20
    } else if (type === "object") {
        if (a === null) return b === null
        if (b === null) return false
        if (Array.isArray(a)) {
            if (!Array.isArray(b)) return false
            const len = a.length
            if (len != b.length) return false
            for (let i = 0; i < a.length; i++) {
                if (!deepEqual(a[i], b[i])) return false
            }
            return true
        } else {
            for (const key of Object.keys(a)) {
                if (!b.hasOwnProperty(key)) return false
                if (!deepEqual(a[key], b[key])) return false
            }
            for (const key of Object.keys(b)) {
                if (!a.hasOwnProperty(key)) return false
            }
            return true
        }
    } else {
        return a === b
    }
}

export function getTime() {
    return performance.now() * (1.0 / 1000.0)
}

