# litemap — AI Agent Quick Ref

Zero-allocation compact sorted flat-array map with O(log N) binary search and deterministic ordered iteration (Rust ICU4X litemap parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SearchResult,
    StrEntry,
    StrInsertResult,
    U64Entry,
    U64InsertResult,
    LiteMapStr,
    LiteMapU64,
    cmpStr,
} from "litemap";
```

## Core Types & Constants

```cp
class SearchResult { found: bool, index: usize }
class StrEntry { key: string, value: string }
class StrInsertResult { ok: bool, replaced: bool, prev: string }
class U64Entry { key: u64, value: i64 }
class U64InsertResult { ok: bool, replaced: bool, prev: i64 }
class LiteMapStr { entries: [LITEMAP_CAP]StrEntry, count: usize } // methods: init(...) +10
class LiteMapU64 { entries: [LITEMAP_CAP]U64Entry, count: usize } // methods: init(...) +10
```

## API Signatures

```cp
fn cmpStr(a: string, b: string): i8;
```

## Minimal Usage

```cp
import { cmpStr } from "litemap";

pub fn main() {
    let result = cmpStr("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"litemap"`; install via `cap add litemap`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
