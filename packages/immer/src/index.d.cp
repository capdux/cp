// Type definitions for CP

export enum PatchOp {
    Add,
    Replace,
    Remove
}

export class StatePatch {
    op: PatchOp,
    path: string,
    value: string,
    prev_value: string,
}

export fn immpWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn patchOpName(op: PatchOp): string;
export fn invertPatch(p: StatePatch): StatePatch;
export fn formatJsonPointer(segments: &[]string, out: []u8): usize;
export fn parseJsonPointer(ptr: string, out: []string): usize;
export fn unescapePointerToken(token: string, out: []u8): usize;
