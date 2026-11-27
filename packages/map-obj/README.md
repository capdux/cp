[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# map-obj

Zero-heap fixed-pool object key-value map/filter/transform utility with skip sentinel and deep key path support (npm map-obj equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:map-obj` ([https://github.com/sindresorhus/map-obj](https://github.com/sindresorhus/map-obj))

| Metadata | Value |
| --- | --- |
| Package | `map-obj` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add map-obj
```

```cp
import {
    ObjEntry,
    MapObjResult,
    mobjStrEq,
    swapKeysAndValues,
    pickKeys,
    omitKeys,
    filterEmptyValues,
    mergeObjects,
} from "map-obj";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ObjEntry` | `class` | Fields: `key: string, val: string, active: bool` |
| `MapObjResult` | `class` | Fields: `entries: [32]ObjEntry, count: usize` · Methods: `empty, push, get` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mobjStrEq` | `(a: string, b: string): bool` | Invokes `mobjStrEq` and returns `bool`. |
| `swapKeysAndValues` | `(src: MapObjResult): MapObjResult` | Invokes `swapKeysAndValues` and returns `MapObjResult`. |
| `pickKeys` | `(src: MapObjResult, allow_keys: []string): MapObjResult` | Invokes `pickKeys` and returns `MapObjResult`. |
| `omitKeys` | `(src: MapObjResult, skip_keys: []string): MapObjResult` | Invokes `omitKeys` and returns `MapObjResult`. |
| `filterEmptyValues` | `(src: MapObjResult): MapObjResult` | Invokes `filterEmptyValues` and returns `MapObjResult`. |
| `mergeObjects` | `(base: MapObjResult, overrides: MapObjResult): MapObjResult` | Invokes `mergeObjects` and returns `MapObjResult`. |

## Minimal Example

```cp
import { mobjStrEq } from "map-obj";

pub fn main() {
    let result = mobjStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
