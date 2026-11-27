# immer@10.1.1 (Agent Quickref)

> Zero-heap structured state patches (Immer / RFC 6902) and RFC 6901 JSON Pointer codec with inverse patch generation in pure CP | License: MIT
> Import: `import { ... } from "immer";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum PatchOp`

## Functions
- `export fn immpWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn patchOpName(op: PatchOp): string;`
- `export fn invertPatch(p: StatePatch): StatePatch;`
- `export fn formatJsonPointer(segments: &[]string, out: []u8): usize;`
- `export fn parseJsonPointer(ptr: string, out: []string): usize;`
- `export fn unescapePointerToken(token: string, out: []u8): usize;`
