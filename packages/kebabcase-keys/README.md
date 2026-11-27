[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# kebabcase-keys

Zero-heap object key converter to `kebab-case` with exclude lists, deep dot-path conversion with `stop_paths`, string/key/path helpers, and batch conversion in pure CP.

- **Package**: `kebabcase-keys` (`v1.0.0`)
- **Upstream Alignment**: Clean-room CP port of `npm/kebabcase-keys` (`MIT`, `sindresorhus/kebabcase-keys`), composing `map-obj` (`MapObjResult`) and `decamelize-keys` (`DecamelizeKeysResult`).

## Installation & Import

```bash
cap add kebabcase-keys
```

```typescript
import { MapObjResult } from "map-obj";
import { DecamelizeKeysResult } from "decamelize-keys";
import {
    kebabcaseString, isKebabCase,
    kebabcaseKeys, kebabcaseKeysAll, kebabcaseEntries,
    kebabcaseKeysDeep, kebabcaseKeyInto, kebabcasePathInto,
    kebabcaseKeysBatch,
} from "kebabcase-keys";
```

## Core API

### Object & Batch Key Conversion

| Function | Signature | Description |
| --- | --- | --- |
| `kebabcaseKeys` | `(src: MapObjResult, exclude: []string): DecamelizeKeysResult` | Converts keys of `src` to `kebab-case`, skipping keys listed in `exclude`. |
| `kebabcaseKeysAll` | `(src: MapObjResult): DecamelizeKeysResult` | Converts all keys of `src` to `kebab-case` with no exclusions. |
| `kebabcaseEntries` | `(keys: &[]string, vals: &[]string, exclude: &[]string): DecamelizeKeysResult` | Converts parallel key/value slices into a `DecamelizeKeysResult`. |
| `kebabcaseKeysDeep` | `(src: MapObjResult, exclude: &[]string, stop_paths: &[]string): DecamelizeKeysResult` | Converts dot-separated nested path keys (e.g. `"fooBar.bazQux"` -> `"foo-bar.baz-qux"`), respecting `exclude` and `stop_paths` prefixes. |
| `kebabcaseKeysBatch` | `(items: []MapObjResult, exclude: []string, out: []DecamelizeKeysResult): usize` | Batch-converts an array of `MapObjResult` into `out`; returns count written. |

### String, Single-Key & Dot-Path Utilities

| Function | Signature | Description |
| --- | --- | --- |
| `kebabcaseString` / `kebabcaseKeyInto` | `(input: string, out: []u8): usize` | Converts a single camelCase/PascalCase/snake_case string to `kebab-case` into `out`. |
| `kebabcasePathInto` | `(dot_path: string, out: []u8): usize` | Converts each `.`-delimited segment of `dot_path` to `kebab-case` into `out`. |
| `isKebabCase` | `(s: string): bool` | Returns `true` if `s` is valid lowercase `kebab-case`. |

## Examples

### 1. Convert Object Keys with Exclusions

```typescript
import { MapObjResult } from "map-obj";
import { kebabcaseKeys } from "kebabcase-keys";

fn main() {
    let obj = MapObjResult.empty();
    obj.push("fooBar", "1");
    obj.push("userId", "42");
    obj.push("keepMe", "raw");

    let excl: [1]string = ["keepMe"];
    let res = kebabcaseKeys(obj, excl[0..]);
    // res.get("foo-bar") == "1", res.get("user-id") == "42", res.get("keepMe") == "raw"
}
```

### 2. Deep Dot-Path Keys with `stop_paths`

```typescript
import { MapObjResult } from "map-obj";
import { kebabcaseKeysDeep } from "kebabcase-keys";

fn main() {
    let obj = MapObjResult.empty();
    obj.push("userProfile.firstName", "Ada");
    obj.push("rawMeta.camelKey", "preserve");

    let excl: [0]string = [];
    let stops: [1]string = ["rawMeta"];
    let res = kebabcaseKeysDeep(obj, excl[0..], stops[0..]);
    // "user-profile.first-name" converted; "rawMeta.camelKey" preserved
}
```

### 3. Direct String & Path Formatting

```typescript
import { kebabcaseString, kebabcasePathInto, isKebabCase } from "kebabcase-keys";

fn main() {
    let buf: [64]u8 = undefined;
    let n = kebabcaseString("XMLHttpRequest", buf[0..]); // "xml-http-request"
    let ok = isKebabCase(buf[0..n]);                     // true

    let pbuf: [64]u8 = undefined;
    let m = kebabcasePathInto("apiConfig.baseUrl", pbuf[0..]); // "api-config.base-url"
}
```

## Differences from Upstream & Highlights

- **Zero Heap Allocation**: Uses fixed-capacity stack structs `MapObjResult` (up to 32 entries) and `DecamelizeKeysResult` (up to 32 entries / 1024-byte key arena) from `map-obj` and `decamelize-keys`.
- **Flat Dot-Path Nesting**: Represents deep object keys as dot-separated paths (`kebabcaseKeysDeep`) with `stop_paths` prefix guards, avoiding recursive dynamic JSON trees.
- **No Regex Engine Overhead**: Pure single-pass ASCII casing state machine with acronym handling.
