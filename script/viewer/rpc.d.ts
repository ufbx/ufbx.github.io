export function rpcOnReady(cb: () => void): void
export function rpcSetup(): void
export function rpcReload(): void
export function rpcDestroy(): void
export function rpcCall(input: any): any
export function rpcMemory(): ArrayBuffer
export function rpcHeapU8(): Uint8Array
export function rpcGl(): WebGL2RenderingContext
export function rpcLoadScene(name: string, buffer: ArrayBuffer): any