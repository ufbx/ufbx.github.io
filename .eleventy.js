const CleanCSS = require("clean-css")
const MarkdownIt = require("markdown-it")
const eleventyNavigationPlugin = require("@11ty/eleventy-navigation")
const eleventyTocPlugin = require('eleventy-plugin-nesting-toc')
const anchor = require('markdown-it-anchor')
const fs = require("fs")
const url = require("url")
const string = require('string')
const { execSync } = require("child_process")
const { highlight } = require("./site/ufbx-highlight")
const Prism = require("prismjs")
const loadLanguages = require("prismjs/components/")

loadLanguages(["rust", "bash"])

global.ufbxReflection = null

let globalContext = {
  prefix: "",
  locals: [],
}

module.exports = function(eleventyConfig) {
  
  eleventyConfig.addPlugin(eleventyNavigationPlugin)
  eleventyConfig.addPlugin(eleventyTocPlugin, {
  })
  
  eleventyConfig.addFilter("cssmin", function(code) {
    return new CleanCSS({}).minify(code).styles
  })
  
  eleventyConfig.addPassthroughCopy("static")
  
  eleventyConfig.addPassthroughCopy({
    "native/build/js_viewer/js_viewer.js": "native/js_viewer.js",
    "native/build/js_viewer/js_viewer.wasm": "native/js_viewer.wasm",
    "native/build/js_viewer/js_viewer.wasm.map": "native/js_viewer.wasm.map",
    "script/style.css": "script/style.css",
    "script/build/bundle.js": "script/bundle.js",
  })

  eleventyConfig.addFilter('flattenNavigationAndAddNextPrev', (nav) => {
    const flat = []
    const visit = (items) => {
      for (const item of items) {
        flat.push(item)
        visit(item.children)
      }
    }
    visit(nav)
    for (let i = 0; i < flat.length; i++) {
      const item = flat[i]
      item.prev = flat[i - 1]
      item.next = flat[i + 1]
    }
    return flat
  })

  const substitutions = [
    [/c\+\+/gi, "cpp"],
  ]

  const md = new MarkdownIt({
    html: true,
  })
    .use(require("markdown-it-footnote"))
    .use(anchor, {
      permalink: anchor.permalink.linkAfterHeader({
        style: "aria-label",
        assistiveText: title => `Permalink to "${title}"`,
        wrapper: ['<div class="header-parent">', '</div>']
      }),
      slugify: s => {
        for (const [re, sub] of substitutions) {
          s = s.replace(re, sub)
        }
        return string(s).slugify().toString()
      },
      tabIndex: false,
    })
  
  md.renderer.rules.fence = (tokens, idx, options, env, self) => {
    let langs = []

    if (idx > 0 && tokens[idx - 1].type === "fence") return ""

    for (let i = idx; i < tokens.length; i++) {
      const token = tokens[i]
      if (token.type !== "fence") break
      langs.push(token)
    }

    let globalPrefix = `langselect-${idx}`

    const langNames = {
      "c": "C",
      "cpp": "C++",
      "rust": "Rust",
      "bash": "Bash",
    }

    function renderTab(token, index) {
      const lang = token.info
      const name = langNames[lang]
      const id = `${globalPrefix}-${lang}`
      let content = token.content
      const checked = index === 0 ? "checked" : ""

      const match = content.match(/^\s*(\/\/|#)\s*ufbx-doc-example\s*:[^\n]*\n/m)
      if (match) {
        content = content.substring(match[0].length)
      }

      if (!lang) {
        return `<pre class="lang-content">${content}</pre>`
      }

      let highlightedContent = ""
      if (lang === "c" || lang === "cpp") {
        highlightedContent = highlight(content)
      } else if (lang === "rust") {
        highlightedContent = Prism.highlight(content, Prism.languages.rust, "rust")
      } else if (lang === "bash") {
        highlightedContent = Prism.highlight(content, Prism.languages.bash, "bash")
      } else {
        throw new Error(`Unknown language: ${lang}`)
      }

      return `
<input type="radio" id="${id}" name="${globalPrefix}" class="lang-radio" ${checked} />
<label for="${id}" class="lang-label">${name}</label>
<pre class="lang-content">${highlightedContent}</pre>`
    }

    return `
<div class="lang-select">
  ${langs.map(renderTab).join("\n")}
</div>`
  }
  
  md.renderer.rules.code_block = (tokens, idx, options, env, self) => {
    const { content } = tokens[idx]
    return `<pre>${highlight(content)}</pre>`
  }
  
  md.renderer.rules.code_inline = (tokens, idx, options, env, self) => {
    const { content } = tokens[idx]
    return `<code>${highlight(content)}</code>`
  }
  
  eleventyConfig.setLibrary("md", md)
  
  eleventyConfig.addLiquidFilter("removeDotHtml", (value) => {
    const v = value.replace(/\.html/g, "")
    return v
  })
  
  // Redirect suffix-less routes to .html
  eleventyConfig.setBrowserSyncConfig({
    server: {
      baseDir: "build",
      middleware: [
        function (req, res, next) {
          const file = "build" + url.parse(req.url).pathname
          if (fs.existsSync(file)) {
            return next()
          }
          
          const htmlFile = file.replace(/\/*$/, ".html")
          if (fs.existsSync(htmlFile)) {
            const content = fs.readFileSync(htmlFile)
            res.setHeader('content-type', 'text/html; charset=UTF-8');
            res.write(content);
            res.writeHead(200);
            res.end()
          } else {
            return next()
          }
        },
      ],
    },
  })
  
  function gatherFields(fields, parent)
  {
    if (parent.kind === "group" || parent.kind === "struct") {
      for (const decl of parent.decls) {
        gatherFields(fields, decl)
      }
    } else if (parent.kind === "decl") {
      const type = parent.type.name
      if (type) {
        fields[parent.name] = type
      }
    }
  }
  
  function gatherEnumValues(info, parent, type)
  {
    if (parent.kind === "group" || parent.kind === "enum") {
      for (const decl of parent.decls) {
        gatherEnumValues(info, decl, type)
      }
    } else if (parent.kind === "decl") {
      info.enumValues[parent.name] = type
    }
  }
  
  function gatherInfo(info, parent)
  {
    if (parent.kind === "group") {
      for (const decl of parent.decls) {
        gatherInfo(info, decl)
      }
    } else if (parent.kind === "struct") {
      if (parent.name) {
        let type = { kind: parent.structKind, fields: { } }
        gatherFields(type.fields, parent)
        info.types[parent.name] = type
      }
    } else if (parent.kind === "enum") {
      gatherEnumValues(info, parent, parent.name)
      info.types[parent.name] = { kind: "enum" }
    } else if (parent.kind === "decl") {
      if (parent.declKind === "typedef") {
        info.types[parent.name] = { kind: "typedef" }
      } else if (parent.isFunction) {
        info.decls[parent.name] = { kind: "function" }
      } else {
        info.decls[parent.name] = { kind: "decl" }
      }
    }
  }
  
  const pythonExecutables = [
    "python3",
    "python",
  ]
  
  const pythonExe = pythonExecutables.find((exe) => {
    try {
      const version = execSync(`${exe} --version`, { encoding: "utf-8" })
      const match = version.match(/Python (\d+)\.(\d+)\.(\d+)/)
      return match && parseInt(match[1]) == 3 && parseInt(match[2]) >= 6
    } catch {
      return false
    }
  })
  
  if (!pythonExe) {
    console.warn("Could not find Python 3.6+, skipping parsing")
  }
  
  eleventyConfig.addWatchTarget("./parser/ufbx_parser.py")
  eleventyConfig.addWatchTarget("./native/viewer/ufbx.h")
  eleventyConfig.on("eleventy.before", async () => {
    if (pythonExe) {
      const cmd = `${pythonExe} parser/ufbx_parser.py -i native/viewer/ufbx.h -o parser/build/ufbx.json`
      console.log(cmd)
      execSync(cmd)
    }
    
    global.ufbxReflection = JSON.parse(fs.readFileSync("parser/build/ufbx.json"))
    
    const info = { types: { }, enumValues: { }, decls: { } }
    for (const decl of global.ufbxReflection) {
      gatherInfo(info, decl)
    }
    global.ufbxInfo = info
  })
  
  return {
    passthroughCopy: true,
    dir: {
      input: "site",
      output: "build",
    }
  }
}