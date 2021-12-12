
export function v3(x, y, z) { return { x, y, z } }
export function add3(a, b) { return { x: a.x+b.x, y: a.y+b.y, z: a.z+b.z } }
export function sub3(a, b) { return { x: a.x-b.x, y: a.y-b.y, z: a.z-b.z } }
export function mul3(a, b) { return { x: a.x*b, y: a.y*b, z: a.z*b } }
export function div3(a, b) { return { x: a.x/b, y: a.y/b, z: a.z/b } }
export function mad3(a, b, c) { return { x: a.x+b.x*c, y: a.y+b.y*c, z: a.z+b.z*c } }
export function dot3(a, b) { return a.x*b.x + a.y*b.y + a.z*b.z }
export function length3(a) { return Math.sqrt(a.x*a.x + a.y*a.y + a.z*a.z) }
export function normalize3(a) { return div3(a, length3(a)) }
export function cross3(a, b) { return { x: a.y*b.z - a.z*b.y, y: a.z*b.x - a.x*b.z, z: a.x*b.y - a.y*b.x } }
