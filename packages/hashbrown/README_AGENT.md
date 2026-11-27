# hashbrown — AI Agent Quick Ref

Zero-heap SwissTable-style control-byte hash map and hash set (clean-room CP implementation of crates.io #1 hashbrown) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    HbGetResult,
    HbInsertResult,
    HashMapU64,
    HashSetU64,
    hbHashU64,
    hbH2,
} from "hashbrown";
```

## Core Types & Constants

```cp
class HbGetResult { found: bool, value: i64 }
class HbInsertResult { ok: bool, replaced: bool, old_value: i64 }
class HashMapU64 { ctrl: [128]u8, keys: [128]u64, vals: [128]i64, used: usize } // methods: new(...) +9
class HashSetU64 { inner: HashMapU64 } // methods: new(...) +6
```

## API Signatures

```cp
fn hbHashU64(key: u64): u64;
fn hbH2(h: u64): u8;
```

## Minimal Usage

```cp
import { hbHashU64 } from "hashbrown";

pub fn main() {
    let result = hbHashU64(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"hashbrown"`; install via `cap add hashbrown`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
