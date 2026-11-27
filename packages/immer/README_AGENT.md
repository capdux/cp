# immer — AI Agent Quick Ref

- **Import**: `import { ImmerState, Draft, produce, produceWithPatches, applyPatches, freeze, isFrozen, ... } from "immer";`
- **Origin**: npm `immer` v10.1.1 (MIT, zero-heap, capacity = 64 entries / 64 patches)
- **Full Types**: `src/index.d.cp`

## Core Types
```cp
enum PatchOp { Add, Replace, Remove }
class StatePatch { op: PatchOp, path: string, value: string, prev_value: string }
class StateEntry { key: string, value: string }
class ImmerState {
    entries: [64]StateEntry, len: usize, frozen: bool,
    fn new(): ImmerState;
    fn count(): usize;
    fn get(key: string): string?;
    fn has(key: string): bool;
    fn setUnfrozen(key: string, value: string): bool;
    fn removeUnfrozen(key: string): bool;
}
class Draft {
    state: ImmerState, patches: [64]StatePatch, patch_count: usize,
    fn fromState(base: ImmerState): Draft;
    fn get(key: string): string?;
    fn has(key: string): bool;
    fn set(key: string, value: string): bool;
    fn remove(key: string): bool;
    fn finish(): ImmerState;
}
class ProduceResult {
    next_state: ImmerState,
    patches: [64]StatePatch, patch_count: usize,
    inverse_patches: [64]StatePatch, inverse_count: usize,
}
```

## Functions
- `produce(base: ImmerState, mutator: (&mut Draft) => void): ImmerState`
- `produceWithPatches(base: ImmerState, mutator: (&mut Draft) => void): ProduceResult`
- `applyPatches(base: ImmerState, patches: &[]StatePatch): ImmerState`
- `freeze(state: &mut ImmerState)` | `isFrozen(state: ImmerState): bool`
- `patchOpName(op: PatchOp): string` | `invertPatch(p: StatePatch): StatePatch`
- `inversePatches(patches: &[]StatePatch, out: []StatePatch): usize`
- `formatPatchJson(p: StatePatch, out: []u8): usize`
- `formatJsonPointer(segments: &[]string, out: []u8): usize`
- `parseJsonPointer(ptr: string, out: []string): usize`
- `unescapePointerToken(token: string, out: []u8): usize`

## Gotchas
1. Always slice `&res.patches[0..res.patch_count]` and `&res.inverse_patches[0..res.inverse_count]` before passing to `applyPatches`.
2. `ImmerState` holds up to **64** string key-value entries; `Draft` records up to **64** patches.
3. Once `freeze(&mut state)` is called (or returned from `produce`), `setUnfrozen`/`removeUnfrozen` return `false`.
