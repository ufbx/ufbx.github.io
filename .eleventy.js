const CleanCSS = require("clean-css")
const MarkdownIt = require("markdown-it")
const eleventyNavigationPlugin = require("@11ty/eleventy-navigation")
const fs = require("fs")
const url = require("url")
const { execSync } = require("child_process")
const { highlight } = require("./site/ufbx-highlight")

global.ufbxReflection = null

let globalContext = {
    prefix: "",
    locals: [],
}

module.exports = function(eleventyConfig) {

  eleventyConfig.addPlugin(eleventyNavigationPlugin)

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

  const md = new MarkdownIt({
    html: true,
  })

  md.renderer.rules.fence = (tokens, idx, options, env, self) => {
      const { content } = tokens[idx]
      return `<pre>${highlight(content)}</pre>`
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
      const cmd = `${pythonExe} parser/ufbx_parser.py`
      console.log(cmd)
      execSync(cmd)
    }

    global.ufbxReflection = JSON.parse(fs.readFileSync("parser/build/ufbx.json"))
  })

  return {
    passthroughCopy: true,
    dir: {
      input: "site",
      output: "build",
    }
  }
}