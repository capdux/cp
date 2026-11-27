[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# immer

Zero-heap immutable state updates (`produce`, `produceWithPatches`, `applyPatches`), RFC 6902 state patches with inverse patch generation, and RFC 6901 JSON Pointer codec in pure CP.

- **Upstream Alignment**: Modeled after npm [`immer`](https://github.com/immerjs/immer) `10.1.1` (`MIT`).
- **Package Version**: `10.1.1`

## Installation & Import

```bash
cap add immer
```

In `cap.json`:

```json
{
  "dependencies": {
    "immer": "^10.1.1"
  }
}
```

```typescript
import {
    ImmerState, Draft, ProduceResult, StatePatch, PatchOp,
    produce, produceWithPatches, applyPatches, freeze, isFrozen,
    invertPatch, inversePatches, formatJsonPointer, parseJsonPointer,
    unescapePointerToken, formatPatchJson, patchOpName,
    IMMER_MAX_ENTRIES, IMMER_MAX_PATCHES
} from "immer";
```

## Core API Reference

### Constants, Enums & Data Structures

| Symbol | Definition | Description |
| :--- | :--- | :--- |
| `IMMER_MAX_ENTRIES` / `IMMER_MAX_PATCHES` | `64` / `64` | Fixed stack capacity for state entries and recorded patches |
| `PatchOp` | `enum { Add, Replace, Remove }` | RFC 6902 patch operation kind |
| `StatePatch` | `class { op: PatchOp, path: string, value: string, prev_value: string }` | Structured patch with previous value for inversion |
| `StateEntry` | `class { key: string, value: string }` | Key-value pair stored in `ImmerState` |
| `ProduceResult` | `class { next_state: ImmerState, patches: [64]StatePatch, patch_count: usize, inverse_patches: [64]StatePatch, inverse_count: usize }` | Returned by `produceWithPatches` |

### `class ImmerState` & `class Draft`

| Symbol | Methods | Description |
| :--- | :--- | :--- |
| `ImmerState` | `new()`, `count(): usize`, `get(key: string): string?`, `has(key: string): bool`, `setUnfrozen(*_, key, value): bool`, `removeUnfrozen(*_, key): bool` | Immutable-when-frozen key-value state container (max 64 entries) |
| `Draft` | `fromState(base: ImmerState): Draft`, `get(key): string?`, `has(key): bool`, `set(*_, key, value): bool`, `remove(*_, key): bool`, `finish(): ImmerState` | Mutable draft copy that records `StatePatch` entries on every mutation |

### Core Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `produce` | `produce(base: ImmerState, mutator: (&mut Draft) => void): ImmerState` | Produce a new frozen `ImmerState` without mutating `base` |
| `produceWithPatches` | `produceWithPatches(base: ImmerState, mutator: (&mut Draft) => void): ProduceResult` | Produce next state along with forward and inverse patches |
| `applyPatches` | `applyPatches(base: ImmerState, patches: &[]StatePatch): ImmerState` | Apply a slice of patches to `base` and return the resulting state |
| `freeze` / `isFrozen` | `freeze(state: &mut ImmerState)` / `isFrozen(state: ImmerState): bool` | Freeze an `ImmerState` or inspect its frozen flag |
| `invertPatch` | `invertPatch(p: StatePatch): StatePatch` | Invert a single patch (`Add <-> Remove`, `Replace` swaps `value`/`prev_value`) |
| `inversePatches` | `inversePatches(patches: &[]StatePatch, out: []StatePatch): usize` | Generate reversed inverse patches into `out` |
| `patchOpName` | `patchOpName(op: PatchOp): string` | Return `"add"`, `"replace"`, or `"remove"` |
| `formatJsonPointer` | `formatJsonPointer(segments: &[]string, out: []u8): usize` | Encode segments into an RFC 6901 JSON Pointer (`~` -> `~0`, `/` -> `~1`) |
| `parseJsonPointer` | `parseJsonPointer(ptr: string, out: []string): usize` | Split an RFC 6901 JSON Pointer into raw segment slices |
| `unescapePointerToken` | `unescapePointerToken(token: string, out: []u8): usize` | Unescape a single RFC 6901 token (`~1` -> `/`, `~0` -> `~`) |
| `formatPatchJson` | `formatPatchJson(p: StatePatch, out: []u8): usize` | Serialize a `StatePatch` as RFC 6902 JSON into `out` |

## Examples

### 1. Immutable Update with `produceWithPatches` & Undo

```typescript
import { ImmerState, Draft, produceWithPatches, applyPatches } from "immer";

fn demoUndo(): bool {
    let mut base = ImmerState.new();
    base.setUnfrozen("theme", "light");
    base.setUnfrozen("lang", "en");

    let res = produceWithPatches(base, (draft: &mut Draft) => {
        draft.set("theme", "dark");
        draft.set("font", "mono");
    });

    // Undo by applying inverse patches
    let restored = applyPatches(res.next_state, res.inverse_patches[0..res.inverse_count]);
    let theme = restored.get("theme") orelse "";
    return theme == "light" && !restored.has("font");
}
```

### 2. RFC 6901 JSON Pointer Encoding & Patch Formatting

```typescript
import { formatJsonPointer, formatPatchJson, StatePatch, PatchOp } from "immer";

fn encodePointerAndPatch(): usize {
    let segs = ["users", "a/b", "profile~1"];
    let mut ptr_buf: [64]u8 = undefined;
    let ptr_len = formatJsonPointer(&segs, &mut ptr_buf);
    // "/users/a~1b/profile~01"

    let p = StatePatch { op: PatchOp.Replace, path: "/theme", value: "dark", prev_value: "light" };
    let mut json_buf: [128]u8 = undefined;
    let json_len = formatPatchJson(p, &mut json_buf);
    return ptr_len + json_len;
}
```

## Differences from Upstream & CP Highlights

- **Zero-Heap Copy-on-Write**: `ImmerState` and `Draft` store up to 64 entries and 64 patches inline on the stack—no JS `Proxy` overhead and no heap allocation.
- **Borrow-Checked Draft Mutation**: `mutator: (&mut Draft) => void` enforces exclusive mutable access to the draft while keeping `base` untouched.
- **Built-in RFC 6901/6902 Serialization**: Includes zero-allocation JSON Pointer escaping/unescaping and patch JSON formatting into caller-provided `[]u8` buffers.
