# is-plain-obj — AI Agent Quick Ref

Zero-heap plain object metadata checker, JSON object literal validator, and prototype-pollution key guard (clean-room port of npm is-plain-obj) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ValueMeta,
    ipoIsSpace,
    ipoSliceEq,
    ipoIsDangerousSlice,
    isPlainObjectMeta,
    isPlainJsonObject,
    isPlainObjectKey,
    hasDangerousProtoKey,
} from "is-plain-obj";
```

## Core Types & Constants

```cp
class ValueMeta { proto_kind: u8, has_to_string_tag: bool, is_iterator: bool, is_null: bool } // methods: plain(...) +1
```

## API Signatures

```cp
fn ipoIsSpace(ch: u8): bool;
fn ipoSliceEq(s: string, start: usize, end: usize, target: string): bool;
fn ipoIsDangerousSlice(s: string, start: usize, end: usize): bool;
fn isPlainObjectMeta(m: ValueMeta): bool;
fn isPlainJsonObject(json_text: string): bool;
fn isPlainObjectKey(key: string): bool;
fn hasDangerousProtoKey(json_text: string): bool;
```

## Minimal Usage

```cp
import { isPlainJsonObject } from "is-plain-obj";

pub fn main() {
    let result = isPlainJsonObject("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"is-plain-obj"`; install via `cap add is-plain-obj`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
