# immer — AI Agent Quick Ref

Zero-heap immutable state updates (`produce`, `produceWithPatches`, `applyPatches`), RFC 6902 patches, and RFC 6901 JSON Pointer codec.

## Import
```typescript
import {
    ImmerState, Draft, ProduceResult, StatePatch, PatchOp,
    produce, produceWithPatches, applyPatches, freeze, isFrozen,
    invertPatch, inversePatches, formatJsonPointer, parseJsonPointer,
    unescapePointerToken, formatPatchJson, patchOpName,
    IMMER_MAX_ENTRIES, IMMER_MAX_PATCHES
} from "immer";
```

## API Signatures
```typescript
export const IMMER_MAX_ENTRIES: usize = 64;
export const IMMER_MAX_PATCHES: usize = 64;
export enum PatchOp { Add, Replace, Remove }
export class StatePatch { op: PatchOp, path: string, value: string, prev_value: string }
export class StateEntry { key: string, value: string }
export class ImmerState {
    entries: [64]StateEntry, len: usize, frozen: bool,
    fn new(): ImmerState;
    fn count(): usize;
    fn get(key: string): string?;
    fn has(key: string): bool;
    fn setUnfrozen(*_, key: string, value: string): bool;
    fn removeUnfrozen(*_, key: string): bool;
}
export class Draft {
    state: ImmerState, patches: [64]StatePatch, patch_count: usize,
    fn fromState(base: ImmerState): Draft;
    fn get(key: string): string?;
    fn has(key: string): bool;
    fn set(*_, key: string, value: string): bool;
    fn remove(*_, key: string): bool;
    fn finish(): ImmerState;
}
export class ProduceResult {
    next_state: ImmerState, patches: [64]StatePatch, patch_count: usize,
    inverse_patches: [64]StatePatch, inverse_count: usize,
}
export fn produce(base: ImmerState, mutator: (&mut Draft) => void): ImmerState;
export fn produceWithPatches(base: ImmerState, mutator: (&mut Draft) => void): ProduceResult;
export fn applyPatches(base: ImmerState, patches: &[]StatePatch): ImmerState;
export fn freeze(state: &mut ImmerState);
export fn isFrozen(state: ImmerState): bool;
export fn patchOpName(op: PatchOp): string;
export fn invertPatch(p: StatePatch): StatePatch;
export fn inversePatches(patches: &[]StatePatch, out: []StatePatch): usize;
export fn formatJsonPointer(segments: &[]string, out: []u8): usize;
export fn parseJsonPointer(ptr: string, out: []string): usize;
export fn unescapePointerToken(token: string, out: []u8): usize;
export fn formatPatchJson(p: StatePatch, out: []u8): usize;
```

## Minimal Example
```typescript
import { ImmerState, Draft, produceWithPatches, applyPatches } from "immer";

fn demo(): string {
    let mut s = ImmerState.new();
    s.setUnfrozen("mode", "dev");
    let r = produceWithPatches(s, (d: &mut Draft) => { d.set("mode", "prod"); });
    let undone = applyPatches(r.next_state, r.inverse_patches[0..r.inverse_count]);
    return undone.get("mode") orelse "";
}
```

## Pitfalls & Rules
- Fixed inline limits: `IMMER_MAX_ENTRIES = 64` and `IMMER_MAX_PATCHES = 64`.
- `ImmerState.setUnfrozen` / `removeUnfrozen` return `false` if `state.frozen == true` (always true on states returned by `produce`).
- Slice `r.patches[0..r.patch_count]` / `r.inverse_patches[0..r.inverse_count]` before passing `&[]StatePatch` to `applyPatches`.
- Output buffers (`out: []u8`, `out: []StatePatch`, `out: []string`) are caller-owned; return value is written count/bytes.
