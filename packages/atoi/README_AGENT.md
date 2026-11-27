# atoi — AI Agent Quick Ref

Zero-heap overflow-checked integer parsing directly from byte slices (base 10, 16, 2..36) matching Rust atoi (262M+ downloads) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ParseU64Result,
    ParseI64Result,
    digitVal,
    fromRadixU64,
    fromRadix10U64,
    fromRadix16U64,
    fromRadixI64,
    fromRadix10I64,
} from "atoi";
```

## Core Types & Constants

```cp
class ParseU64Result { value: u64, used: usize, ok: bool }
class ParseI64Result { value: i64, used: usize, ok: bool }
```

## API Signatures

```cp
fn digitVal(ch: u8, radix: u32): i32;
fn fromRadixU64(bytes: string, radix: u32): ParseU64Result;
fn fromRadix10U64(bytes: string): ParseU64Result;
fn fromRadix16U64(bytes: string): ParseU64Result;
fn fromRadixI64(bytes: string, radix: u32): ParseI64Result;
fn fromRadix10I64(bytes: string): ParseI64Result;
fn atoiU64(s: string): ParseU64Result;
fn atoiI64(s: string): ParseI64Result;
```

## Minimal Usage

```cp
import { fromRadixU64 } from "atoi";

pub fn main() {
    let result = fromRadixU64("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"atoi"`; install via `cap add atoi`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
