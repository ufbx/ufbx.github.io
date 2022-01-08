const CleanCSS = require("clean-css");
const eleventyNavigationPlugin = require("@11ty/eleventy-navigation");

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

  return {
    passthroughCopy: true,
    dir: {
      input: "site",
      output: "build",
    }
  }
}