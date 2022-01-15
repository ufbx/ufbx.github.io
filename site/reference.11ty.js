const MarkdownIt = require("markdown-it")

const md = new MarkdownIt({
    html: true,
    linkify: true,
})

function linkRefs(str) {
    return str.replace(/((ufbx_|UFBX_)[A-Za-z0-9_\.]+)[A-Za-z0-9_/]*/, (sub, root) => {
        return `<a href="#${root}">${sub}</a>`
    })
}

md.renderer.rules.code_inline = (tokens, idx, options, env, self) => {
    const { content } = tokens[idx]
    return `<code>${linkRefs(content)}</code>`
}

function renderComment(comment) {
    return md.render(comment.join("\n"))
}

function formatType(type) {
    let s = type.name
    for (const mod of type.mods) {
        switch (mod.type) {
        case "pointer": s = `${s}*`; break
        case "const": s = `const ${s}`; break
        case "inline": s = `ufbx_inline ${s}`; break
        case "array": s = `${s}[${mod.length}]`; break
        }
    }
    return s
}

function renderType(type) {
    return md.render(comment.join("\n"))
}

function renderDecl(decl) {
    if (decl.kind === "paragraph") {
        return [renderComment(decl.comment), "\n"]
    } else if (decl.kind === "struct") {
        let result = []
        result.push(`<h3 class="ref-type" id=${decl.name}>${decl.name}</h3>`)
        result.push(renderComment(decl.comment))
        result.push(`<div class="struct-body">`)
        for (const field of decl.decls) {
            if (field.kind === "paragraph") {
                result.push(renderComment(field.comment))
                result.push("\n")
            }
            if (!field.name) continue
            const typeStr = linkRefs(formatType(field.type))
            result.push(`<h4 class="ref-field" id="${decl.name}.${field.name}"><span class="ref-fname">${field.name}</span> <span class="ref-ftype">${typeStr}</span></h4>`)
            result.push(renderComment(field.comment))
        }
        result.push(`</div>`)
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
    render({ ufbxReflection }) {
        return ufbxReflection.map(renderDecl).flat().join("\n")
    }
}

module.exports = Page
