let globalContext = {
    prefix: "",
    locals: [],
    inReference: false,
}

const tokenTypes = {
    comment: "//[^\\n]*|/\\*.*?\\*/",
    name: "[A-Za-z_\$][A-Za-z_0-9\$]*(?:::[A-Za-z_\$][A-Za-z_0-9\$]*)*",
    string: "\"(?:\\\"|[^\"])*?\"",
    header: "<[a-zA-Z0-9_\\.]+>(?=\\s*\\n|\\s*$)",
    preproc: "#\\w+",
    line: "\n",
    space: "[ \\t\\r]+",
    op: "(?:->|<<|>>|::|:|@\\(|\\(|\\)|.)",
}

const tokenNames = Object.keys(tokenTypes)
const tokenRegex = new RegExp(tokenNames.map(n => `(${tokenTypes[n]})`).join("|"), "g")

const keywords = new Set([
    "const",
    "if",
    "else",
    "for",
    "while",
    "return",
    "struct",
    "union",
    "typedef",
    "inline",
])

const builtins = new Set([
    "true",
    "false",
    "nullptr",
    "NULL",
])

const types = new Set([
    "bool",
    "int",
    "unsigned",
    "short",
    "long",
    "char",
    "void",
    "uint8_t",
    "int8_t",
    "uint16_t",
    "int16_t",
    "uint32_t",
    "int32_t",
    "uint64_t",
    "int64_t",
    "size_t",
    "ptrdiff_t",
    "float",
    "double",
    "T",
])

const advancedTypes = new Set([
    "std::vector",
    "Vector2",
    "Vector3",
    "Vector4",
    "Matrix4",
])

function tokenize(source) {
    const re = new RegExp(tokenRegex)
    let match = null
    let tokens = [{ type: "line", text: "" }]
    let hide = false
    while ((match = re.exec(source)) !== null) {
        for (let i = 0; i < tokenNames.length; i++) {
            const text = match[i + 1]
            if (text) {
                const type = tokenNames[i]
                if (text == "@(") {
                    hide = true
                } else if (hide && text == ")") {
                    hide = false
                } else {
                    tokens.push({ type, text, hide })
                }
                break
            }
        }
    }
    return tokens
}

function search(tokens, pattern) {
    const re = new RegExp(pattern, "dg")

    let parts = []
    let mapping = []
    let pos = 0, index = 0
    for (const token of tokens) {
        if (token.type === "comment" || token.type === "space") {
            index += 1
            continue
        }

        mapping[pos] = index
        let repr = ""
        if (token.type === "string" || token.type === "line" || token.type === "header") {
            repr = `${token.type} `
        } else {
            repr = `${token.type}:${token.text} `
        }
        parts.push(repr)
        for (let i = 0; i < repr.length; i++) {
            mapping[pos++] = index
        }
        index += 1
    }

    const str = parts.join("")

    let results = []
    let match = null
    while ((match = re.exec(str)) !== null) {
        const pos = mapping[match.index]
        results.push({
            pos: pos,
            token: tokens[pos],
            groups: (match.indices ?? []).map((pair) => {
                const begin = mapping[pair[0]]
                const end = mapping[pair[1]]
                return {
                    begin, end,
                    token: tokens[begin],
                    tokens: tokens.slice(begin, end),
                }
            })
        })
        re.lastIndex -= match[0].length - 1;
    }

    return results
}

function patchKeywords(tokens) {
    for (const m of search(tokens, /name/)) {
        if (keywords.has(m.token.text)) {
            m.token.type = "kw"
        } else if (builtins.has(m.token.text)) {
            m.token.type = "builtin"
        }
    }
}

function patchTypes(tokens) {
    const { ufbxInfo } = global
    for (const m of search(tokens, /name/)) {
        const text = m.token.text
        if (types.has(text)) {
            m.token.type = "type"
        } else if (advancedTypes.has(text)) {
            m.token.type = "type"
            m.token.viewType = "type-cpp"
        } else if (m.token.text in ufbxInfo.types) {
            m.token.type = "type"
            m.token.refType = m.token.text
            m.token.ufbxType = ufbxInfo.types[text]
        }
    }
}

let globalDeclId = 0

function patchDecls(tokens) {
    for (const m of search(tokens, /line (?:kw:const )?(type:\S* )(?:op:< (?:type:\S+ (?:op:, )?)*op:> )?(?:op:\* )*(name:\S* )(?:op:= |op:; |op:\[ )/)) {
        const type = m.groups[1].token
        const name = m.groups[2].token
        name.declType = type.text
        name.declId = ++globalDeclId
    }
    for (const m of search(tokens, /(?:op:\( |op:, )(?:kw:const )?(type:\S* )(?:op:< (?:type:\S+ (?:op:, )?)*op:> )?(?:op:\* )*(name:\S* )(?:op:= |op:; |op:, |op:\) |op:\[ |op:: )/)) {
        const type = m.groups[1].token
        const name = m.groups[2].token
        name.declType = type.text
        name.declId = ++globalDeclId
    }
}

function patchRefs(tokens) {
    let scopes = [{ }]
    let prevToken = { type: "line", text: "" }
    for (let i = 0; i < tokens.length; i++) {
        const token = tokens[i]
        if (token.text === "{") {
            scopes.push({ })
        } else if (token.text === "}") {
            scopes.pop()
        } else if (token.declType) {
            const scope = scopes[scopes.length - 1]
            scope[token.text] = { type: token.declType, id: token.declId }
        } else if (token.type === "name" && prevToken.text !== "." && prevToken.text !== "->") {
            for (let i = scopes.length - 1; i >= 0; i--) {
                const scope = scopes[i]
                const ref = scope[token.text]
                if (ref) {
                    token.refType = ref.type
                    token.refId = ref.id
                    break;
                }
            }
        }

        prevToken = token
    }
}

function patchFields(tokens) {
    const { ufbxInfo } = global
    for (const m of search(tokens, /(name:\S* )(?:op:\. |op:-> )(name:\S* )/)) {
        const parent = m.groups[1].token
        const field = m.groups[2].token
        if (parent.refType) {
            field.structType = parent.refType
            const ufbxType = ufbxInfo.types[parent.refType]
            if (ufbxType && ufbxType.kind === "struct") {
                const ufbxField = ufbxType.fields[field.text]
                if (ufbxField) {
                    field.refType = ufbxField
                }
            }
        }
    }
    for (const m of search(tokens, /(type:\S* )op:\. (name:\S* )/)) {
        const parent = m.groups[1].token
        const field = m.groups[2].token
        if (parent.refType) {
            field.structType = parent.text
        }
    }
}

function patchInitFields(tokens) {
    for (const m of search(tokens, /line op:\. (name:\S* )/)) {
        const name = m.groups[1].token
        let pos = m.groups[1].begin
        let numOpen = 0
        for (; pos >= 0; pos--) {
            const tok = tokens[pos]
            if (tok.text === "{") {
                numOpen += 1
            } else if (tok.type === "line" && numOpen > 0) {
                break
            } else if (tok.declType) {
                name.structType = tok.declType
                break
            }
        }
    }
}

function patchCalls(tokens) {
    for (const m of search(tokens, /(name:\S* )(op:\( )/)) {
        let pos = m.groups[1].begin
        tokens[pos].isCall = true
    }
}

function linkRef(ref) {
    if (globalContext.inReference) {
        return `#${ref}`
    } else {
        return `/reference#${ref}`
    }
}

function highlight(str) {
    const tokens = tokenize(str)
    patchKeywords(tokens)
    patchTypes(tokens)
    patchDecls(tokens)
    patchRefs(tokens)
    patchFields(tokens)
    patchFields(tokens)
    patchInitFields(tokens)
    patchCalls(tokens)

    let prevTokenNext = null
    return tokens.map((token) => {
        let prevToken = prevTokenNext
        prevTokenNext = token
        if (token.hide) return ""

        const safeText = token.text.replace("<", "&lt;").replace(">", "&gt;")
        if (token.type === "op" || token.type === "line" || token.type === "space") {
            return safeText
        } else if (token.type === "header") {
            return `<span class="c-header">${safeText}</span>`
        } else if (token.type === "comment") {
            return `<span class="c-comment">${safeText}</span>`
        } else {
            const type = token.viewType ?? token.type

            let classes = [`c-${type}`]
            let tag = "span"
            let attribs = { }
            if (token.declId) attribs["data-decl-id"] = token.declId
            if (token.refId) attribs["data-ref-id"] = token.refId
            if (token.isCall) classes.push("c-call")
            attribs["class"] = classes.join(" ")

            let refText = token.text
            if (token.type === "name" && prevToken && prevToken.hide && prevToken.type === "name") {
                refText = prevToken.text + refText
            }

            let doRef = true
            if (refText.startsWith("ufbx_") && refText.toLowerCase() != refText) {
                doRef = false
            }

            if (doRef) {
                if (refText.toLocaleLowerCase().startsWith("ufbx_")) {
                    tag = "a"
                    attribs["href"] = linkRef(refText.toLowerCase())
                } else if (token.structType && token.structType.startsWith("ufbx_")) {
                    tag = "a"
                    attribs["href"] = linkRef(`${token.structType}.${refText}`)
                }
            }

            const attribStr = Object.keys(attribs).map(key => `${key}="${attribs[key]}"`).join(" ")
            return `<${tag} ${attribStr}>${safeText}</${tag}>`
        }
    }).join("")
}

function setHighlightContext(ctx) {
    const prev = globalContext
    globalContext = ctx
    return prev
}

module.exports = {
    highlight,
    setHighlightContext,
}
