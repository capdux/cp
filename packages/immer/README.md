[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# immer

Zero-heap immutable state producer (`produce` / `produceWithPatches` / `applyPatches`), freeze guards (`freeze` / `isFrozen`), RFC 6902 state patches with automatic inverse patch generation, and RFC 6901 JSON Pointer codec in pure CP.

## Source & Zero-Cost Migration

| Field | Value |
| --- | --- |
| **Upstream Registry** | [npmjs.com/package/immer](https://www.npmjs.com/package/immer) |
| **Upstream Package** | `immer` v10.1.1 |
| **Upstream Repository** | https://github.com/immerjs/immer |
| **License** | MIT |

Keep the exact package name when migrating from TypeScript/npm to CP:

```ts
// TypeScript (npm)
import { produce, produceWithPatches, applyPatches, freeze } from "immer";
```

```cp
// CP — same package name, zero-heap immutable state transitions & RFC 6902 patches
import {
    ImmerState, Draft, produce, produceWithPatches,
    applyPatches, freeze, isFrozen,
} from "immer";
```

## Installation

```bash
cap add immer
```

## Quick Start & Practical Examples

### 1. Immutable Updates with `produce` & Freeze Protection

```cp
import { ImmerState, Draft, produce, freeze, isFrozen } from "immer";

fn demoProduce() {
    mut base = ImmerState.new();
    let _ = base.setUnfrozen("theme", "light");
    let _ = base.setUnfrozen("lang", "en");
    freeze(&mut base); // isFrozen(base) == true; further setUnfrozen/removeUnfrozen return false

    // Produce a new immutable state without mutating base
    let next = produce(base, |draft: &mut Draft| {
        let _ = draft.set("theme", "dark");
        let _ = draft.set("sidebar", "open");
        let _ = draft.remove("lang");
    });

    // base still has theme == "light", lang == "en"
    // next has theme == "dark", sidebar == "open", has("lang") == false
    let cur = next.get("theme");
}
```

### 2. Time-Travel Undo/Redo with `produceWithPatches` & `applyPatches`

```cp
import {
    ImmerState, Draft, ProduceResult,
    produceWithPatches, applyPatches,
    formatPatchJson, inversePatches, invertPatch,
} from "immer";

fn demoPatches() {
    mut base = ImmerState.new();
    let _ = base.setUnfrozen("count", "1");

    let res: ProduceResult = produceWithPatches(base, |draft: &mut Draft| {
        let _ = draft.set("count", "2");
        let _ = draft.set("status", "active");
    });

    // Forward replay onto base reproduces res.next_state
    let replayed = applyPatches(base, &res.patches[0..res.patch_count]);

    // Inverse replay onto next_state restores base (undo)
    let undone = applyPatches(res.next_state, &res.inverse_patches[0..res.inverse_count]);

    // Serialize a patch to RFC 6902 JSON into a stack buffer
    mut json_buf: [256]u8 = [0; 256];
    if (res.patch_count > 0) {
        let written = formatPatchJson(res.patches[0], &mut json_buf);
    }
}
```

### 3. RFC 6901 JSON Pointer Codec

```cp
import { formatJsonPointer, parseJsonPointer, unescapePointerToken } from "immer";

fn demoJsonPointer() {
    let segs: [3]string = ["users", "a/b", "c~d"];
    mut ptr_buf: [128]u8 = [0; 128];
    let n = formatJsonPointer(&segs, &mut ptr_buf); // "/users/a~1b/c~0d"

    mut parsed: [8]string = [""; 8];
    let count = parseJsonPointer("/users/a~1b/c~0d", &mut parsed);

    mut unescaped: [32]u8 = [0; 32];
    let ulen = unescapePointerToken("a~1b", &mut unescaped); // "a/b"
}
```

## Performance & Safety Highlights

- **Zero Heap Allocations**: `ImmerState` (`[64]StateEntry`), `Draft` (`[64]StatePatch`), and `ProduceResult` live entirely on the stack with deterministic memory footprint.
- **Immutability Enforcement**: `produce` and `produceWithPatches` return frozen states (`isFrozen(next) == true`), preventing accidental post-hoc mutation.
- **Automatic Inverse Patches**: `produceWithPatches` records both forward patches and exact inverse patches (`Add` <-> `Remove`, `Replace` <-> `Replace` with `prev_value`) for zero-cost undo/redo.
- **RFC 6901 Escaping**: `formatJsonPointer` and `unescapePointerToken` properly handle `~0` (`~`) and `~1` (`/`) escape sequences.

## API Reference

See [`src/index.d.cp`](src/index.d.cp) for complete type signatures, or [`README_AGENT.md`](README_AGENT.md) for the compact AI-agent reference.
