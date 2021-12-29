
export const elementTypeCategory = {
    node: "node",
    mesh: "attrib",
    light: "attrib",
    camera: "attrib",
    bone: "attrib",
    empty: "attrib",
    material: "shading",
    texture: "shading",
    skin_deformer: "deformer",
    skin_cluster: "deformer",
    blend_deformer: "deformer",
    blend_channel: "deformer",
    blend_shape: "deformer",
    cache_deformer: "deformer",
    cache_file: "deformer",
}

export function typeToIconUrl(type) {
    return `/static/icons/element/${type}.svg`
}
