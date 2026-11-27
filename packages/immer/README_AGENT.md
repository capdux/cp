# immer@10.1.1 (Agent Quickref)

> Zero-heap structured state patches (Immer / RFC 6902) and RFC 6901 JSON Pointer codec with inverse patch generation in pure CP | License: MIT
> Import: `import { ... } from "immer";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum PatchOp`

## Functions
- `export fn immpStrEq(a: string, b: string): bool;`
- `export fn immpWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn immpWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn immpNormalizeKey(path: string): string;`
- `export fn immpEmptyPatch(): StatePatch;`
- `export fn patchOpName(op: PatchOp): string;`
- `export fn invertPatch(p: StatePatch): StatePatch;`
- `export fn inversePatches(patches: &[]StatePatch, out: []StatePatch): usize;`
- `export fn formatJsonPointer(segments: &[]string, out: []u8): usize;`
- `export fn parseJsonPointer(ptr: string, out: []string): usize;`
- `export fn unescapePointerToken(token: string, out: []u8): usize;`
- `export fn formatPatchJson(p: StatePatch, out: []u8): usize;`
- `export fn freeze(state: &mut ImmerState);`
- `export fn isFrozen(state: ImmerState): bool;`
- `export fn produce(base: ImmerState, mutator: (&mut Draft) => void): ImmerState;`
- `export fn produceWithPatches(base: ImmerState, mutator: (&mut Draft) => void): ProduceResult;`
- `export fn applyPatches(base: ImmerState, patches: &[]StatePatch): ImmerState;`
