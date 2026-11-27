# map-obj — AI Agent Quick Ref

Zero-heap fixed-pool object key-value map/filter/transform utility with skip sentinel and deep key path support (npm map-obj equivalent) (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class ObjEntry { key: string, val: string, active: bool }
class MapObjResult { entries: [32]ObjEntry, count: usize } // methods: empty(...) +2
```

## API Signatures

```cp
fn mobjStrEq(a: string, b: string): bool;
fn swapKeysAndValues(src: MapObjResult): MapObjResult;
fn pickKeys(src: MapObjResult, allow_keys: []string): MapObjResult;
fn omitKeys(src: MapObjResult, skip_keys: []string): MapObjResult;
fn filterEmptyValues(src: MapObjResult): MapObjResult;
fn mergeObjects(base: MapObjResult, overrides: MapObjResult): MapObjResult;
```

## Minimal Usage

```cp
import { mobjStrEq } from "map-obj";

pub fn main() {
    let result = mobjStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"map-obj"`; install via `cap add map-obj`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
