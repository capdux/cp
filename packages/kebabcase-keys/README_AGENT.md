# kebabcase-keys (AI Agent Quick Ref)

Zero-heap object key & dot-path converter to `kebab-case`, composing `map-obj` and `decamelize-keys`.

## Import
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

## API Signatures
- `kebabcaseString(input: string, out: []u8): usize` / `kebabcaseKeyInto(key: string, out: []u8): usize`
- `kebabcasePathInto(dot_path: string, out: []u8): usize`
- `isKebabCase(s: string): bool`
- `kebabcaseKeys(src: MapObjResult, exclude: []string): DecamelizeKeysResult`
- `kebabcaseKeysAll(src: MapObjResult): DecamelizeKeysResult`
- `kebabcaseEntries(keys: &[]string, vals: &[]string, exclude: &[]string): DecamelizeKeysResult`
- `kebabcaseKeysDeep(src: MapObjResult, exclude: &[]string, stop_paths: &[]string): DecamelizeKeysResult`
- `kebabcaseKeysBatch(items: []MapObjResult, exclude: []string, out: []DecamelizeKeysResult): usize`

## Minimal Example
```typescript
import { MapObjResult } from "map-obj";
import { kebabcaseKeysAll } from "kebabcase-keys";

fn convertOne(): string? {
    let obj = MapObjResult.empty();
    obj.push("firstName", "Ada");
    let res = kebabcaseKeysAll(obj);
    return res.get("first-name");
}
```

## Pitfalls & Borrowing Rules
- `MapObjResult` and `DecamelizeKeysResult` are fixed stack structs: max `32` entries and `1024` total converted key bytes in `DecamelizeKeysResult.key_buf`.
- Notice slice mutability in signatures: `kebabcaseKeys` and `kebabcaseKeysBatch` take `exclude: []string` (and `items: []MapObjResult`), while `kebabcaseEntries` and `kebabcaseKeysDeep` take read-only `&[]string`.
- `res.get("kebab-key")` returns `string?`; always unwrap with `if (let v = res.get(...))`.
- For `kebabcaseString` / `kebabcasePathInto`, allocate `out.len >= input.len * 2` in case hyphens are inserted between consecutive uppercase/lowercase transitions.
- `stop_paths` in `kebabcaseKeysDeep` matches exact path or prefix followed by `.` (e.g. `"rawMeta"` stops `"rawMeta.camelKey"`).
