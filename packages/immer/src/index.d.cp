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

export class StateEntry {
    key: string,
    value: string,
}

export fn immpStrEq(a: string, b: string): bool;
export fn immpWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn immpWriteStr(out: []u8, pos: usize, s: string): usize;
export fn immpNormalizeKey(path: string): string;
export fn immpEmptyPatch(): StatePatch;
export fn patchOpName(op: PatchOp): string;
export fn invertPatch(p: StatePatch): StatePatch;
export fn inversePatches(patches: &[]StatePatch, out: []StatePatch): usize;
export fn formatJsonPointer(segments: &[]string, out: []u8): usize;
export fn parseJsonPointer(ptr: string, out: []string): usize;
export fn unescapePointerToken(token: string, out: []u8): usize;
export fn formatPatchJson(p: StatePatch, out: []u8): usize;
export class ImmerState {
    entries: [64]StateEntry,
    len: usize,
    frozen: bool,
    fn new(): ImmerState;
    fn count(): usize;
    fn get(key: string): string?;
    fn has(key: string): bool;
    fn setUnfrozen(_: auto, key: string, value: string): bool;
    fn removeUnfrozen(_: auto, key: string): bool;
}

export fn freeze(state: &mut ImmerState);
export fn isFrozen(state: ImmerState): bool;
export class Draft {
    state: ImmerState,
    patches: [64]StatePatch,
    patch_count: usize,
    fn fromState(base: ImmerState): Draft;
    fn get(key: string): string?;
    fn has(key: string): bool;
    fn set(_: auto, key: string, value: string): bool;
    fn remove(_: auto, key: string): bool;
    fn finish(): ImmerState;
}

export class ProduceResult {
    next_state: ImmerState,
    patches: [64]StatePatch,
    patch_count: usize,
    inverse_patches: [64]StatePatch,
    inverse_count: usize,
}

export fn produce(base: ImmerState, mutator: (&mut Draft) => void): ImmerState;
export fn produceWithPatches(base: ImmerState, mutator: (&mut Draft) => void): ProduceResult;
export fn applyPatches(base: ImmerState, patches: &[]StatePatch): ImmerState;
