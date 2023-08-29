const MarkdownIt = require("markdown-it")
const { highlight, setHighlightContext } = require("./ufbx-highlight")

const md = new MarkdownIt({
    html: true,
    linkify: true,
})

const forceTableEnums = new Set([])

const ignoredDecls = new Set([
    "ufbx_inline",
    "UFBX_UFBX_H_INLCUDED",
    "UFBX_LIST_TYPE",
    "UFBX_VERTEX_ATTRIB_IMPL",
    "UFBX_CALLBACK_IMPL",
])

md.renderer.rules.code_block = (tokens, idx, options, env, self) => {
    const { content } = tokens[idx]
    return `<pre>${highlight(content)}</pre>`
}

md.renderer.rules.code_inline = (tokens, idx, options, env, self) => {
    const { content } = tokens[idx]
    return `<code>${highlight(content)}</code>`
}

function parseComment(comment, pos=0, type="text") {
    let lines = []
    const start = pos

    if (pos >= comment.length) return []

    // Heuristic: If the comment has a sentence ending in the first line
    // separate it to it's own paragraph.
    if (pos === 0 && comment[0].endsWith(".")) {
        lines.push(comment[0])
        return [{ type, lines }, ...parseComment(comment, 1)]
    }

    if (type === "code") {
        for (; pos < comment.length; pos++) {
            const line = comment[pos]
            if (!line.startsWith("  ")) {
                let followedByCode = false
                for (let scan = pos; scan < comment.length; scan++) {
                    const lookahead = comment[scan]
                    if (lookahead === "") {
                        continue
                    } else if (lookahead.startsWith("  ")) {
                        followedByCode = true
                        break
                    } else {
                        break
                    }
                }
                if (followedByCode) {
                    lines.push("    ")
                } else {
                    break
                }
            } else {
                lines.push("  " + line)
            }
        }
        return [{ type, lines }, ...parseComment(comment, pos)]
    } else {
        let nextType = "text"
        for (; pos < comment.length; pos++) {
            const line = comment[pos]
            if (line.startsWith("  ")) {
                nextType = "code"
                break
            } else if (line.startsWith("NOTE:") && (type !== "note" || pos > start)) {
                nextType = "note"
                break
            } else if (line.startsWith("HINT:") && (type !== "hint" || pos > start)) {
                nextType = "hint"
                break
            }
            lines.push(line)
        }

        // Trim the `NOTE: ` prefix.
        if (type === "note" && lines.length > 0) {
            lines[0] = lines[0].replace("NOTE: ", "")
        } else if (type === "hint" && lines.length > 0) {
            lines[0] = lines[0].replace("HINT: ", "")
        }

        return [{ type, lines }, ...parseComment(comment, pos, nextType)]
    }
}

const patches = [
    { from: /\b([dn])\^([2-3])/g, to: "$1<sup>$2</sup>" },
]

function renderComment(comment) {
    if (comment && comment.length > 0) {
        return parseComment(comment).map(({ type, lines }) => {
            let html = md.render(lines.join("\n"))

            for (const { from, to } of patches) {
                html = html.replace(from, to)
            }

            return `<div class="desc-${type}">${html}</div>`
        }).join("\n")
    } else {
        return ""
    }
}

function renderDescComment(comment, showNoDesc, className="desc") {
    if (comment && comment.length > 0) {
        return `<div class="${className}">${renderComment(comment)}</div>`
    } else if (showNoDesc) {
        return `<div class="no-desc"></div>`
    } else {
        return []
    }
}

function formatType(type) {
    let s = type.name
    for (const mod of type.mods) {
        switch (mod.type) {
        case "pointer": s = `${s}*`; break
        case "const": s = `const\xa0${s}`; break
        case "array": s = `${s}[${mod.length}]`; break
        }
    }
    return s
}

function isSelfTypedef(decl) {
    if (decl.kind !== "decl") return false
    if (decl.declKind !== "typedef") return false
    if (decl.type.name !== decl.name) return false
    return true
}

function isDefine(decl) {
    if (decl.kind !== "decl") return false
    if (decl.declKind !== "define") return false
    return true
 }

function shouldRender(decl) {
    if (decl.kind === "group") {
        if (decl.decls.every(isSelfTypedef)) return false
    }
    return true
}

function isDefineGroup(decl) {
    if (decl.kind === "group") {
        if (decl.decls.some(isDefine)) return true
    }
    return false
}

function shouldIgnoreDeclGroup(decl) {
    if (decl.kind === "group") {
        let ignoredDeclName = null
        let nonIgnoredDeclName = null
        for (const inner of decl.decls) {
            if (inner.kind !== "decl") continue
            if (ignoredDecls.has(inner.name)) {
                ignoredDeclName = inner.name
            } else {
                nonIgnoredDeclName = inner.name
            }
        }
        if (ignoredDeclName && nonIgnoredDeclName) {
            throw new Error(`Group has both ignored decl '${ignoredDeclName}' and non-ignored decl '${nonIgnoredDeclName}'`)
        }
        if (ignoredDeclName) return true
    }
    return false
}

function shouldIgnoreFieldGroup(decl) {
    if (decl.kind === "group") {
        let ignoredDeclName = null
        let nonIgnoredDeclName = null
        for (const inner of decl.decls) {
            if (inner.kind !== "decl") continue
            if (inner.name.startsWith("_")) {
                ignoredDeclName = inner.name
            } else {
                nonIgnoredDeclName = inner.name
            }
        }
        if (ignoredDeclName && nonIgnoredDeclName) {
            throw new Error(`Group has both ignored decl '${ignoredDeclName}' and non-ignored decl '${nonIgnoredDeclName}'`)
        }
        if (ignoredDeclName) return true
    }
    return false
}

function renderType(type) {
    return highlight(formatType(type))
}

function structContext(decl) {
    let locals = []
    for (const field of decl.decls) {
        if (field.kind === "decl") {
            locals.push(field.name)
        } else if (field.kind === "group") {
            for (const innerField of field.decls) {
                locals.push(innerField.name)
            }
        }
    }

    return { prefix: `${decl.name}.`, locals, inReference: true }
}

function renderDeclGroup(parentName, decl, nested) {
    let r = []
    if (decl.kind === "group") {
        if (shouldIgnoreFieldGroup(decl)) return r

        r.push(`<div class="field-group">`)
        r.push(`<div class="field">`)
        r.push(`<table class="code field-decls" role="presentation">`)
        for (const inner of decl.decls) {
            const typeStr = highlight(formatType(inner.type))
            const id = `${parentName}.${inner.name}`
            r.push(`<tr class="field-row">`)
            r.push(`<td class="field-type">${typeStr}\xa0</td>`)
            r.push(`<td class="field-name"><a class="field-link" id="${id}" href="#${id}">${inner.name}</td>`)
            r.push(`</tr>`)
        }
        r.push(`</table>`)
        r.push(`</div>`)
        r.push(renderDescComment(decl.comment, !nested))
        r.push(`</div>`)
    } else if (decl.kind === "struct") {
        r.push(`<div class="field struct-nested">`)
        if (decl.name) {
            r.push(`<div class="code nested-head"><span class="kw">${decl.structKind}</span>\xa0${decl.name}</div>`)
        } else {
            r.push(`<div class="code nested-head"><span class="kw">${decl.structKind}</span></div>`)
        }

        if (nested) {
            r.push(renderDescComment(decl.comment, false))
        }

        r.push(`<div class="nested-fields">`)
        for (const field of decl.decls) {
            r.push(...renderDeclGroup(parentName, field, true))
        }
        r.push(`</div>`)

        if (!nested) {
            r.push(renderDescComment(decl.comment, true))
        }

        r.push(`</div>`)
    }
    return r
}

let previousWasSection = true

function renderDecl(decl) {
    if (decl.kind === "struct") {
        let r = []
        if (!previousWasSection) {
            r.push("</div>")
            r.push("</div>")
            previousWasSection = true
        }

        const ctx = structContext(decl)
        const prevCtx = setHighlightContext(ctx)

        r.push(`<div class="decl struct section">`)
        {
            r.push(`<div id="${decl.name}"></div>`)
            r.push(`<h3 class="section-name"><span class="kww">struct</span> <a class="section-link" href="#${decl.name}">${decl.name}</a></h3>`)

            r.push(`<div class="section-content">`)
            r.push(renderDescComment(decl.comment, false, "desc section-desc"))

            r.push(`<div class="section-fields">`)
            for (const field of decl.decls) {
                r.push(...renderDeclGroup(decl.name, field, false))
            }
            r.push(`</div>`)
            r.push(`</div>`)
        }
        r.push(`</div>`)

        setHighlightContext(prevCtx)
        return r
    } else if (decl.kind === "enum") {
        let r = []
        if (!previousWasSection) {
            r.push("</div>")
            r.push("</div>")
            previousWasSection = true
        }

        const groupFitsTable = (group) => {
            if (group.decls.length > 1) {
                if (group.comment.length === 0) return true
                return false
            } else if (group.decls.length === 1) {
                if (group.comment.length === 0) return true
                if (group.commentInline) return true
                return false
            } else {
                return true
            }
        }

        let useTable = decl.decls.every(groupFitsTable)
        if (forceTableEnums.has(decl.name)) {
            useTable = true
        }

        r.push(`<div class="decl enum section">`)
        r.push(`<div id="${decl.name}"></div>`)
        r.push(`<h3 class="section-name"><span class="kww">enum</span> <a class="section-link" href="#${decl.name}">${decl.name}</a></h3>`)

        r.push(`<div class="section-content">`)
        r.push(renderDescComment(decl.comment, false, "desc section-desc"))

        const { ufbxReflection } = global
        const enumDecl = ufbxReflection.find(d => d.kind === "enumType" && d.enumName === decl.name)

        r.push(`<div class="section-fields">`)
        if (useTable) {
            r.push(`<table class="enum-values">`)
            for (const group of decl.decls) {
                for (const field of group.decls) {
                    if (!field.name) continue
                    if (field.name.includes("_FORCE_32BIT")) continue

                    const id = field.name.toLowerCase()
                    r.push("<tr>")
                    r.push(`<td class="code enum-name"><a class="field-link" id="${id}" href="#${id}">${field.name}</a></td>`)
                    r.push("<td>")
                    r.push(renderComment(group.comment))
                    r.push("</td>")
                    r.push("</tr>")
                }
            }

            if (enumDecl) {
                const id = enumDecl.countName.toLowerCase();
                r.push("<tr>")
                r.push(`<td class="code enum-name enum-count"><a class="field-link" id="${id}" href="#${id}">${enumDecl.countName}</a></td>`)
                r.push("</tr>")
            }

            r.push(`</table>`)
        } else {
            for (const group of decl.decls) {
                for (const inner of group.decls) {
                    const id = inner.name.toLowerCase()
                    if (inner.name.includes("_FORCE_32BIT")) continue
                    r.push(`<div class="code enum-name"><a class="field-link" id="${id}" href="#${id}">${inner.name}</a></div>`)
                }
                r.push(renderDescComment(group.comment, true))
            }

            if (enumDecl) {
                const id = enumDecl.countName.toLowerCase();
                r.push(`<div class="code enum-name enum-count"><a class="field-link" id="${id}" href="#${id}">${enumDecl.countName}</a></div>`)
            }
        }
        r.push(`</div>`)

        r.push(`</div>`)
        r.push(`</div>`)

        return r
    } else if (decl.kind === "group") {
        let r = []
        if (previousWasSection) {
            r.push(`<div class="section">`)
            r.push(`<h3 class="section-name">\xa0</h3>`)
            r.push(`<div class="section-fields section-globals">`)
            previousWasSection = false
        }

        if (shouldIgnoreDeclGroup(decl)) {
            // Nop
        } else if (decl.isFunction) {
            r.push(`<div class="decl toplevel">`)
            for (const field of decl.decls) {
                const type = field.type
                const func = type.mods.find(mod => mod.type === "function")
                const id = field.name

                let proto = `<div class="func"><div class="func-begin">`
                if (field.declKind === "typedef") {
                    proto += `<span class="kw">typedef</span>\xa0`
                } else if (field.declKind === "extern") {
                    proto += `<span class="kw">extern</span>\xa0`
                }
                proto += renderType(type)
                proto += ` <a href="#${id}" class="func-name field-link">${field.name}</a>(</div><div class="func-args">`
                let first = true
                for (const arg of func.args) {
                    if (!first) {
                        proto += ", "
                    }
                    first = false
                    proto += `${renderType(arg.type)}\xa0${arg.name}`
                }
                proto += ")</div></div>"

                r.push(`<div id="${id}" class="code decl-name func-link">${proto}</div>`)

            }

            r.push(renderDescComment(decl.comment, true, "desc top-desc"))
            r.push(`</div>`)
        } else if (isDefineGroup(decl)) {
            r.push(`<div class="decl toplevel">`)
            {
                r.push(`<table class="code field-decls" role="presentation">`)
                for (const inner of decl.decls) {
                    const argString = inner.defineArgs ? inner.defineArgs.join(", ") : ""
                    const id = inner.name.toLowerCase()
                    r.push(`<tr class="field-row">`)
                    r.push(`<td class="field-type c-preproc">#define\xa0</td>`)
                    r.push(`<td><span class="field-name"><a id="${id}" href="#${id}" class="field-link">${inner.name}</a></span>${argString}\xa0</td>`)
                    r.push(`</tr>`)
                }
                r.push(`</table>`)
                r.push(renderDescComment(decl.comment, true, "desc top-desc"))
            }
            r.push(`</div>`)
        } else if (shouldRender(decl)) {
            r.push(`<div class="decl toplevel">`)
            {
                r.push(`<table class="code field-decls" role="presentation">`)
                for (const inner of decl.decls) {
                    const typeStr = highlight(formatType(inner.type))
                    const id = inner.name.toLowerCase()
                    let prefix = ""
                    if (inner.declKind === "extern") {
                        prefix = `<span class="kw">extern</span>\xa0`
                    } else if (inner.declKind === "typedef") {
                        prefix = `<span class="kw">typedef</span>\xa0`
                    }
                    r.push(`<tr class="field-row">`)
                    r.push(`<td class="field-type">${prefix}${typeStr}\xa0</td>`)
                    r.push(`<td class="field-name"><a id="${id}" href="#${id}" class="field-link">${inner.name}</a></td>`)
                    r.push(`</tr>`)
                }
                r.push(`</table>`)
                r.push(renderDescComment(decl.comment, true, "desc top-desc"))
            }
            r.push(`</div>`)
        }

        return r
    }
}

class Page {
    data() {
        return {
            permalink: "/reference.html",
            layout: "layouts/reference",
            title: "Reference",
            eleventyNavigation: {
                key: "Reference",
                order: 300,
            }
        }
    }
    render() {
        const prevCtx = setHighlightContext({
            prefix: "",
            locals: [],
            inReference: true,
        })
        const { ufbxReflection } = global
        let lines = ufbxReflection.map(renderDecl).flat()
        if (!previousWasSection) {
            lines.push("</div>")
            lines.push("</div>")
        }
        setHighlightContext(prevCtx)
        return lines.join("\n")
    }
}

module.exports = Page
