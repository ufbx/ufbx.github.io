const MarkdownIt = require("markdown-it")

const md = new MarkdownIt({
    html: true,
    linkify: true,
})

let globalContext = {
    prefix: "",
    locals: [],
}

const keywords = [
    "extern",
    "typedef",
    "struct",
    "union",
    "enum",
    "const",
    "void",
    "size_t",
    "ptrdiff_t",
    "int32_t",
    "int64_t",
    "uint32_t",
    "uint64_t",
    "bool",
    "char",
    "int",
    "float",
    "double",
    "sizeof",
]

const forceTableEnums = new Set([])

const keywordsPipe = keywords.join("|")
const keywordRegex = new RegExp(`\\b(${keywordsPipe})\\b`, "g")

function linkRefs(str) {
    str = str.replace(/((ufbx_|UFBX_)[A-Za-z0-9_\.]+)[A-Za-z0-9_/]*/g, (sub, root) => {
        return `<a class="dl" href="#${root.toLowerCase()}">${sub}</a>`
    })
    for (const local of globalContext.locals) {
        str = str.replace(new RegExp(`\\b${local}\\b`, "g"), (sub) => {
            const href = globalContext.prefix + local
            return `<a class="ll" href="#${href}">${sub}</a>`
        })
    }
    str = str.replace(keywordRegex, (sub) => {
        return `<span class="kw">${sub}</span>`
    })
    return str
}

md.renderer.rules.code_block = (tokens, idx, options, env, self) => {
    const { content } = tokens[idx]
    return `<pre>${linkRefs(content)}</pre>`
}


md.renderer.rules.code_inline = (tokens, idx, options, env, self) => {
    const { content } = tokens[idx]
    return `<code>${linkRefs(content)}</code>`
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
            if (!line.startsWith("  ")) break
            lines.push("  " + line)
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

function renderComment(comment) {
    if (comment && comment.length > 0) {
        return parseComment(comment).map(({ type, lines }) => {
            const html = md.render(lines.join("\n"))
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
        case "const": s = `const&nbsp;${s}`; break
        case "inline": s = `ufbx_inline&nbsp;${s}`; break
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

function shouldRender(decl) {
    if (decl.kind === "group") {
        if (decl.decls.every(isSelfTypedef)) return false
    }
    return true
}

function renderType(type) {
    return linkRefs(formatType(type))
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

    return { prefix: `${decl.name}.`, locals }
}

function renderDeclGroup(parentName, decl, nested) {
    let r = []
    if (decl.kind === "group") {
        r.push(`<div class="field-group">`)
        r.push(`<div class="field">`)
        r.push(`<table class="code field-decls" role="presentation">`)
        for (const inner of decl.decls) {
            const typeStr = linkRefs(formatType(inner.type))
            const id = `${parentName}.${inner.name}`
            r.push(`<tr class="field-row">`)
            r.push(`<td class="field-type">${typeStr}&nbsp;</td>`)
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
            r.push(`<div class="code nested-head"><span class="kw">${decl.structKind}</span>&nbsp;${decl.name}</div>`)
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

        const prevContext = globalContext
        globalContext = structContext(decl)

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

        globalContext = prevContext
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

        r.push(`<div class="section-fields">`)
        if (useTable) {
            r.push(`<table class="enum-values">`)
            for (const group of decl.decls) {
                for (const field of group.decls) {
                    if (!field.name) continue
                    const id = field.name.toLowerCase()
                    r.push("<tr>")
                    r.push(`<td class="code enum-name"><a class="field-link" id="${id}" href="#${id}">${field.name}</a></td>`)
                    r.push("<td>")
                    r.push(renderComment(group.comment))
                    r.push("</td>")
                    r.push("</tr>")
                }
            }
            r.push(`</table>`)
        } else {
            for (const group of decl.decls) {
                for (const inner of group.decls) {
                    const id = inner.name.toLowerCase()
                    r.push(`<div class="code enum-name"><a class="field-link" id="${id}" href="#${id}">${inner.name}</a></div>`)
                }
                r.push(renderDescComment(group.comment, true))
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
            r.push(`<h3 class="section-name">&nbsp;</h3>`)
            r.push(`<div class="section-fields section-globals">`)
            previousWasSection = false
        }

        if (decl.isFunction) {
            r.push(`<div class="decl toplevel">`)
            for (const field of decl.decls) {
                const type = field.type
                const func = type.mods.find(mod => mod.type === "function")
                const id = field.name

                let proto = `<div class="func"><div class="func-begin">`
                if (field.declKind === "typedef") {
                    proto += `<span class="kw">typedef</span>&nbsp;`
                } else if (field.declKind === "extern") {
                    proto += `<span class="kw">extern</span>&nbsp;`
                }
                proto += renderType(type)
                proto += ` <a id="${id}" href="#${id}" class="func-name field-link">${field.name}</a>(</div><div class="func-args">`
                let first = true
                for (const arg of func.args) {
                    if (!first) {
                        proto += ", "
                    }
                    first = false
                    proto += `${renderType(arg.type)}&nbsp;${arg.name}`
                }
                proto += ")</div></div>"

                r.push(`<div class="code decl-name" id=${field.name}>${proto}</div>`)

            }

            r.push(renderDescComment(decl.comment, true, "desc top-desc"))
            r.push(`</div>`)
        } else if (shouldRender(decl)) {
            r.push(`<div class="decl toplevel">`)
            {
                r.push(`<table class="code field-decls" role="presentation">`)
                for (const inner of decl.decls) {
                    const typeStr = linkRefs(formatType(inner.type))
                    let prefix = ""
                    if (inner.declKind === "extern") {
                        prefix = `<span class="kw">extern</span>&nbsp;`
                    } else if (inner.declKind === "typedef") {
                        prefix = `<span class="kw">typedef</span>&nbsp;`
                    }
                    r.push(`<tr class="field-row" id="${inner.name}">`)
                    r.push(`<td class="field-type">${prefix}${typeStr}&nbsp;</td>`)
                    r.push(`<td class="field-name">${inner.name}</td>`)
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

/*
function renderDecl(decl) {
    if (decl.kind === "paragraph") {
        return [renderComment(decl.comment), "\n"]
    } else if (decl.kind === "struct") {
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

        const prevContext = globalContext
        globalContext = { prefix: `${decl.name}.`, locals }

        let result = []
        result.push(`<div class="s">`)
        result.push(`<h3 class="sn" id=${decl.name}>${decl.name}</h3>`)
        if (decl.comment) {
            result.push(`<div class="d">`)
            result.push(renderComment(decl.comment))
            result.push(`</div>`)
        }

        result.push(`<div class="sc">`)
        for (const field of decl.decls) {
            if (field.kind === "group") {
                result.push(`<div class="sg">`)
                result.push(`<table class="sgf">`)
                for (const inner of field.decls) {
                    const typeStr = linkRefs(formatType(inner.type))
                    result.push(`<tr class="f" id="${decl.name}.${inner.name}">`)
                    result.push(`<td class="n">${inner.name}</td>`)
                    result.push(`<td class="t">${typeStr}</td>`)
                    result.push(`</tr>`)
                }
                result.push(`</table>`)
                if (decl.comment) {
                    result.push(`<div class="d">`)
                    result.push(renderComment(field.comment))
                    result.push(`</div>`)
                }
                result.push(`</div>`)
            } else if (field.kind === "decl") {
                const typeStr = linkRefs(formatType(field.type))
                result.push(`<div class="sf">`)
                result.push(`<div class="nf">`)
                result.push(`<span class="f" id="${decl.name}.${field.name}">`)
                result.push(`<span class="n">${field.name}</span>`)
                result.push(`<span class="t">${typeStr}</td>`)
                result.push(`</span>`)
                result.push(`</div>`)
                if (decl.comment) {
                    result.push(`<div class="d">`)
                    result.push(renderComment(field.comment))
                    result.push(`</div>`)
                }
                result.push(`</div>`)
            }
        }
        result.push(`</div>`)

        result.push(`</div>`)

        globalContext = prevContext
        return result
    }
}
*/

/*
function renderDecl(decl) {
    if (decl.kind === "paragraph") {
        return [renderComment(decl.comment), "\n"]
    } else if (decl.kind === "struct") {
        let locals = []
        for (const field of decl.decls) {
            if (field.name) {
                locals.push(field.name)
            }
        }

        const prevContext = globalContext
        globalContext = { prefix: `${decl.name}.`, locals }

        let result = []
        result.push(`<h3 class="ref-type" id=${decl.name}>${decl.name}</h3>`)
        result.push(renderComment(decl.comment))
        result.push(`<div class="struct-body">`)
        for (const field of decl.decls) {
            if (field.kind === "paragraph") {
                result.push(renderComment(field.comment))
                result.push("\n")
            } else if (field.kind === "group") {
                for (const inner of field.decls) {
                    const typeStr = linkRefs(formatType(inner.type))
                    result.push(`<h4 class="ref-field" id="${decl.name}.${inner.name}"><span class="ref-fname">${inner.name}</span> <span class="ref-ftype">${typeStr}</span></h4>`)
                }
                result.push(renderComment(field.comment))
            } else if (field.kind === "decl") {
                const typeStr = linkRefs(formatType(field.type))
                result.push(`<h4 class="ref-field" id="${decl.name}.${field.name}"><span class="ref-fname">${field.name}</span> <span class="ref-ftype">${typeStr}</span></h4>`)
                result.push(renderComment(field.comment))
            }
        }
        result.push(`</div>`)

        globalContext = prevContext
        return result
    } else if (decl.kind === "enum") {
        let result = []
        result.push(`<h3 class="ref-type" id=${decl.name}>${decl.name}</h3>`)
        result.push(renderComment(decl.comment))
        if (decl.decls.some(d => d.commentInline)) {
            result.push("<table>")
            for (const field of decl.decls) {
                if (!field.name) continue
                result.push("<tr>")
                result.push(`<td class="ref-ename" id="${field.name}">${field.name}</td>`)
                result.push("<td>")
                result.push(renderComment(field.comment))
                result.push("</td>")
                result.push("</tr>")
            }
            result.push("</table>")
        } else {
            for (const field of decl.decls) {
                if (field.kind === "paragraph") {
                    result.push(renderComment(decl.comment))
                    result.push("\n")
                    continue
                }
                if (!field.name) continue
                result.push(`<h4 id="${field.name}">${field.name}</h4>`)
                result.push(renderComment(field.comment))
            }
        }
        return result
    } else if (decl.kind === "decl") {
        let result = []
        const type = decl.type
        if (type && type.name !== decl.name) {
            if (type.mods.some(mod => mod.type === "function")) {
                const typeStr = formatType(decl.type)
                result.push(`<h3 id=${decl.name}>${typeStr} ${decl.name}()</h3>`)
                result.push(renderComment(decl.comment))
                return result
            } else {
                const typeStr = formatType(decl.type)
                result.push(`<h3 id=${decl.name}>${typeStr} ${decl.name}</h3>`)
                result.push(renderComment(decl.comment))
                return result
            }
        }
    }
}
*/

class Page {
    data() {
        return {
            permalink: "/reference.html",
            layout: "layouts/reference",
            eleventyNavigation: {
                key: "Reference",
                order: 1,
            }
        }
    }
    render() {
        const { ufbxReflection } = global
        let lines = ufbxReflection.map(renderDecl).flat()
        if (!previousWasSection) {
            lines.push("</div>")
            lines.push("</div>")
        }
        return lines.join("\n")
    }
}

module.exports = Page