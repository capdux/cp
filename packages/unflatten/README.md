[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# unflatten

Zero-allocation nested JSON flatten and unflatten library for CP, modeled on npm [`flat` / `unflatten`](https://github.com/hughsk/flat) (MIT, CP v1.0.0).

Supports custom key delimiters, maximum recursion depth (`max_depth`), safe array preservation (`safe`), structured key-value table extraction (`FlatTable`), and round-trip nested JSON reconstruction (`unflattenJson`) without a single heap allocation.

## Installation & Import

```bash
cap add unflatten
```

```cp
import {
    FlatOptions,
    FlatTable,
    flattenJson,
    flattenToJson,
    unflattenJson,
} from "unflatten";
```

## Core API Reference

### Configuration & Data Structures

| Type | Fields / Methods | Description |
| :--- | :--- | :--- |
| `FlatOptions` | `delimiter: u8`, `max_depth: usize`, `safe: bool` | Controls flatten/unflatten behavior (`delimiter == 0` defaults to `'.'`). |
| `FlatOptions.defaults()` | `(): FlatOptions` | Returns default options (`delimiter: 46` (`'.'`), `max_depth: 0` (unlimited), `safe: false`). |
| `FlatEntry` | `key: [96]u8`, `key_len: usize`, `val: [128]u8`, `val_len: usize` | Fixed-size flattened key-value entry storing raw JSON value bytes. |
| `FlatTable` | `entries: [48]FlatEntry`, `len: usize` | Fixed-capacity table holding up to 48 flattened JSON entries. |
| `FlatTable.new()` | `(): FlatTable` | Creates an empty `FlatTable`. |
| `FlatTable.set(key, raw_json_val)` | `(key: string, raw_json_val: string): bool` | Inserts or updates a key with raw JSON value bytes; returns `false` if full or oversized. |
| `FlatTable.get(key, out)` | `(key: string, out: []u8): i64` | Copies raw JSON value into `out`, returning byte length (`>= 0`) or `-1` if not found / `out` too small. |
| `FlatTable.count()` | `(): usize` | Returns the number of stored entries. |
| `UfPool` / `UfNode` | `nodes: [96]UfNode`, `count: usize` | Fixed-size tree arena (96 nodes max, 48-byte segment name) used internally for unflattening. |

### High-Level Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `flattenJson` | `(json_in: string, opts: FlatOptions, out_table: &mut FlatTable): i64` | Flattens nested JSON into `out_table`. Returns entry count (`>= 0`) or negative error code. |
| `flattenToJson` | `(json_in: string, opts: FlatOptions, out: []u8): i64` | Flattens nested JSON directly into a flat JSON object in `out`. Returns bytes written (`>= 0`) or `< 0`. |
| `unflattenJson` | `(flat_json: string, opts: FlatOptions, out: []u8): i64` | Reconstructs nested JSON objects/arrays from a flat JSON object into `out`. Returns bytes written (`>= 0`) or `< 0`. |

### Low-Level Parser & Tree Helpers

`unflatten` also exports its zero-allocation scanner and tree-building primitives for custom pipelines: `ufDecodeString`, `ufSkipString`, `ufSkipNumber`, `ufSkipValue`, `ufJoinKey`, `ufFlattenValue`, `ufEmitByte`, `ufEmitBytes`, `ufEmitJsonString`, `ufAllocNode`, `ufFindChild`, `ufAppendChild`, `ufInsertPath`, `ufIsSequentialArray`, `ufEmitNode`, `ufStrEq`, `ufCopyBuf`, `ufIsWs`, `ufIsDigit`, `ufIsHex`, `ufSkipWs`, `ufFormatUsize`, and `ufEmptyEntry`.

## Usage Examples

### 1. Flatten and Unflatten Round-Trip

```cp
import { FlatOptions, flattenToJson, unflattenJson } from "unflatten";

fn roundTripExample() {
    let opts = FlatOptions.defaults();
    let input = "{\"user\":{\"name\":\"Alice\",\"roles\":[\"admin\",\"dev\"]}}";

    let flat_buf: [512]u8 = undefined;
    let flat_len = flattenToJson(input, opts, flat_buf[0..512]);
    if (flat_len >= 0) {
        let flat_str = flat_buf[0..(flat_len as usize)];
        // {"user.name":"Alice","user.roles.0":"admin","user.roles.1":"dev"}

        let nested_buf: [512]u8 = undefined;
        let nested_len = unflattenJson(flat_str, opts, nested_buf[0..512]);
        if (nested_len >= 0) {
            let restored = nested_buf[0..(nested_len as usize)];
            // {"user":{"name":"Alice","roles":["admin","dev"]}}
        }
    }
}
```

### 2. Extract into `FlatTable` with Custom Delimiter & `safe` Arrays

```cp
import { FlatOptions, FlatTable, flattenJson } from "unflatten";

fn tableExample() {
    let opts = FlatOptions {
        delimiter: 47, // '/'
        max_depth: 0,
        safe: true,    // Keep nested arrays intact instead of indexing elements
    };
    let mut table = FlatTable.new();
    let count = flattenJson("{\"cfg\":{\"ports\":[80,443],\"tls\":true}}", opts, &mut table);

    if (count > 0) {
        let val_buf: [128]u8 = undefined;
        let n = table.get("cfg/ports", val_buf[0..128]);
        // n > 0 -> val_buf[0..n] == "[80,443]"
    }
}
```

### 3. Depth-Limited Flattening

```cp
import { FlatOptions, flattenToJson } from "unflatten";

fn maxDepthExample() {
    let opts = FlatOptions {
        delimiter: 46, // '.'
        max_depth: 1,
        safe: false,
    };
    let out: [256]u8 = undefined;
    let n = flattenToJson("{\"a\":{\"b\":{\"c\":1}}}", opts, out[0..256]);
    // Keeps deeper subtrees intact beyond max_depth
}
```

## Differences from Upstream & Zero-Copy Highlights

- **Zero Heap Allocation**: Unlike JavaScript's `flat`/`unflatten`, all parsing, table storage (`48` entries, `96`-byte keys, `128`-byte values), and tree reconstruction (`96` nodes in `UfPool`) live entirely on the stack or caller-supplied `[]u8` buffers.
- **Explicit Error Codes**: Functions return `i64` (`>= 0` for count/length, `< 0` for malformed JSON or capacity overflow) rather than throwing exceptions.
- **Sequential Array Recovery**: During `unflattenJson`, object nodes whose child keys are sequential non-negative integers (`0`, `1`, `2`, ...) are automatically emitted as JSON arrays via `ufIsSequentialArray`.
