# flatted — AI Agent Quick Ref

Zero-allocation circular-reference-safe flattened JSON codec with a 64-node / 256-edge arena graph (npm `flatted` parity).

## Import

```cp
import {
    FlatNodeKind,
    FlatArena,
    stringifyFlatted,
    parseFlatted,
} from "flatted";
```

## Core API Signatures

```cp
// Constants & Types
// FLAT_MAX_NODES = 64, FLAT_MAX_EDGES = 256, FLAT_NONE = -1
enum FlatNodeKind { Null, Bool, Int, Str, Array, Object }

class FlatArena { nodes: [64]FlatNode, node_count: usize, edges: [256]FlatEdge, edge_count: usize }
FlatArena.init(): FlatArena
arena.reset()
arena.nodeCount(): usize
arena.addNull(): usize
arena.addBool(v: bool): usize
arena.addInt(v: i64): usize
arena.addStr(s: string): usize
arena.addArray(): usize
arena.addObject(): usize
arena.setField(obj_id: usize, key: string, child_id: usize): bool
arena.pushElem(arr_id: usize, child_id: usize): bool
arena.nodeKind(id: usize): FlatNodeKind
arena.nodeBool(id: usize): bool
arena.nodeInt(id: usize): i64
arena.nodeStr(id: usize): string
arena.elemCount(arr_id: usize): usize
arena.getField(obj_id: usize, key: string): i64   // -1 if missing
arena.getElem(arr_id: usize, idx: usize): i64      // -1 if OOB

// Codec
stringifyFlatted(arena: FlatArena, root_id: usize, out: []u8): usize
parseFlatted(json: string, out_arena: &mut FlatArena): i64 // >=0 root_id, -1 err
```

## Minimal Example

```cp
import { FlatArena, stringifyFlatted, parseFlatted } from "flatted";

fn demo() {
    let mut a = FlatArena.init();
    let root = a.addObject();
    a.setField(root, "self", root);
    let buf: [256]u8 = undefined;
    let n = stringifyFlatted(a, root, buf[0..256]);
    let mut b = FlatArena.init();
    let r = parseFlatted(buf[0..n], &mut b);
    // r >= 0 && b.getField(r as usize, "self") == r
}
```

## Gotchas & Borrow Rules

- **Arena capacity**: Max 64 nodes and 256 edges; reuse via `arena.reset()`.
- **Borrowing**: `parseFlatted` takes `out_arena: &mut FlatArena`; `stringifyFlatted` takes `arena: FlatArena` by value and writes into writable `out: []u8`.
- **Lifetime**: `parseFlatted` stores zero-copy `string` slices pointing into the input `json`; keep `json` alive while accessing `nodeStr`.
- **Return checks**: `stringifyFlatted` returns `0` on buffer overflow/error; `parseFlatted`, `getField`, and `getElem` return `-1` on failure.
