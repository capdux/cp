[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# flatted

Zero-allocation, circular-reference-safe flattened JSON serializer and parser for CP, modeled on npm [`flatted`](https://github.com/WebReflection/flatted) (ISC, CP v1.0.0).

Uses a fixed-capacity 64-node / 256-edge arena graph (`FlatArena`) to represent arbitrary object and array graphs—including self-referential and mutually recursive cycles—and serializes/deserializes them using the standard `flatted` indexed-array JSON wire format without heap allocations.

## Installation & Import

```bash
cap add flatted
```

```cp
import {
    FlatNodeKind,
    FlatArena,
    stringifyFlatted,
    parseFlatted,
} from "flatted";
```

## Core API Reference

### Constants & Types

| Symbol | Definition | Description |
| :--- | :--- | :--- |
| `FLAT_MAX_NODES` | `usize = 64` | Maximum number of nodes in a `FlatArena`. |
| `FLAT_MAX_EDGES` | `usize = 256` | Maximum number of object-field / array-element edges in a `FlatArena`. |
| `FLAT_NONE` | `i32 = -1` | Sentinel for missing edge links or failed lookups. |
| `FlatNodeKind` | `Null \| Bool \| Int \| Str \| Array \| Object` | Node value discriminant enum. |
| `FlatNode` | `kind`, `bool_val`, `int_val`, `str_val`, `first_edge`, `last_edge`, `edge_count` | Arena graph node entry. |
| `FlatEdge` | `key: string`, `child_id: usize`, `next: i32` | Linked edge connecting a container node to a child node ID. |
| `FltSpan` | `ok: bool`, `start: usize`, `end: usize`, `next: usize` | Parsed string span helper result. |

### `FlatArena` Methods

| Method | Signature | Description |
| :--- | :--- | :--- |
| `FlatArena.init()` | `(): FlatArena` | Initializes an empty 64-node / 256-edge arena. |
| `reset()` | `()` | Clears all nodes and edges for arena reuse. |
| `nodeCount()` | `(): usize` | Returns current allocated node count. |
| `addNull()` / `addBool(v)` / `addInt(v)` / `addStr(s)` | `(...): usize` | Allocates a primitive node and returns its node ID. |
| `addArray()` / `addObject()` | `(): usize` | Allocates a container node and returns its node ID. |
| `setField(obj_id, key, child_id)` | `(usize, string, usize): bool` | Adds or updates an object property edge (supports cycles). |
| `pushElem(arr_id, child_id)` | `(usize, usize): bool` | Appends an element edge to an array node (supports cycles). |
| `nodeKind(id)` | `(usize): FlatNodeKind` | Returns the `FlatNodeKind` of node `id`. |
| `nodeBool(id)` / `nodeInt(id)` / `nodeStr(id)` | `(usize): bool \| i64 \| string` | Reads primitive value of node `id`. |
| `elemCount(arr_id)` | `(usize): usize` | Returns number of elements/edges in a container node. |
| `getField(obj_id, key)` | `(usize, string): i64` | Looks up child node ID by key; returns `-1` if missing. |
| `getElem(arr_id, idx)` | `(usize, usize): i64` | Looks up child node ID by array index; returns `-1` if out of bounds. |

### Codec Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `stringifyFlatted` | `(arena: FlatArena, root_id: usize, out: []u8): usize` | Serializes the graph reachable from `root_id` into `flatted` JSON in `out`. Returns bytes written (`0` on error/overflow). |
| `parseFlatted` | `(json: string, out_arena: &mut FlatArena): i64` | Parses a `flatted` JSON array into `out_arena`. Returns root node ID (`>= 0`) or `-1` on error. |

## Usage Examples

### 1. Serialize a Circular Object Graph

```cp
import { FlatArena, stringifyFlatted } from "flatted";

fn circularSerialize() {
    let mut arena = FlatArena.init();
    let root = arena.addObject();
    let name = arena.addStr("agent-1");
    arena.setField(root, "name", name);
    arena.setField(root, "self", root); // Circular self-reference!

    let out: [512]u8 = undefined;
    let n = stringifyFlatted(arena, root, out[0..512]);
    if (n > 0) {
        let json = out[0..n];
        // [{"name":"1","self":"0"},"agent-1"]
    }
}
```

### 2. Parse `flatted` JSON and Traverse Cycles

```cp
import { FlatArena, parseFlatted } from "flatted";

fn circularParse() {
    let input = "[{\"name\":\"1\",\"self\":\"0\"},\"agent-1\"]";
    let mut arena = FlatArena.init();
    let root_id = parseFlatted(input, &mut arena);

    if (root_id >= 0) {
        let r = root_id as usize;
        let self_id = arena.getField(r, "self");
        // self_id == root_id (cycle restored!)
        let name_id = arena.getField(r, "name");
        if (name_id >= 0) {
            let name = arena.nodeStr(name_id as usize); // "agent-1"
        }
    }
}
```

## Differences from Upstream & Zero-Copy Highlights

- **Explicit Graph Arena**: Because CP has no GC object graph, graphs are constructed explicitly in `FlatArena` via integer node IDs (`0..63`), eliminating pointer cycles and GC overhead.
- **Zero-Copy String Slices**: `parseFlatted` stores string slices directly referencing the input `json` buffer; keep `json` alive while reading `arena.nodeStr(...)`.
- **Bounded Deterministic Memory**: Fixed 64-node / 256-edge arena fits cleanly on the stack and can be reset and reused across requests via `arena.reset()`.
