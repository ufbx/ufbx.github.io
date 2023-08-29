
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

export function deepUnwrap(a) {
    if (Array.isArray(a)) {
        const v = []
        for (let i = 0; i < a.length; i++) {
            v.push(deepUnwrap(a[i]))
        }
        return v
    } else if (typeof a === "object" && a !== null) {
        const v = { }
        for (const key of Object.keys(a)) {
            v[key] = deepUnwrap(a[key])
        }
        return v
    } else {
        return a
    }
}

export function getTime() {
    return performance.now() * (1.0 / 1000.0)
}

