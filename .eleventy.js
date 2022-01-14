const CleanCSS = require("clean-css")
const MarkdownIt = require("markdown-it")
const eleventyNavigationPlugin = require("@11ty/eleventy-navigation")
const fs = require("fs")
const url = require("url")

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

  md.renderer.rules.code_inline = (tokens, idx, options, env, self) => {
    const { content } = tokens[idx]
    const html = content.replace(/(ufbx_[A-Za-z0-9_\.]+)[A-Za-z0-9_/]*(\([^)]*\))?/, (sub, root) => {
      return `<a href="/reference#${root}">${sub}</a>`
    })
    return `<code>${html}</code>`
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

  const ufbxReflection = JSON.parse(fs.readFileSync("parser/build/ufbx.json"))
  eleventyConfig.addGlobalData("ufbxReflection", ufbxReflection)

  return {
    passthroughCopy: true,
    dir: {
      input: "site",
      output: "build",
    }
  }
}