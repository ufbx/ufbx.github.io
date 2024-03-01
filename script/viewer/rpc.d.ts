export function rpcOnReady(cb: () => void): void
export function rpcSetup(): void
export function rpcDestroy(): void
export function rpcCall(input: any): Promise<any>
export function rpcLoadScene(name: string, blob: Blob): Promise<any>