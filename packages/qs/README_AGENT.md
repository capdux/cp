# qs — AI Agent Quick Ref

Zero-allocation query string parser and serializer with nested brackets, dot notation, arrays, strict null handling, and DoS limits for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    QsError,
    QsParseOptions,
    QsPathSegment,
    QsEntry,
    QsParsed,
    QsBuilder,
    isUnreserved,
    needsDecode,
} from "qs";
```

## Core Types & Constants

```cp
error QsError { BufferTooSmall, TooManyParams, DepthExceeded, InvalidPercentEncoding, InvalidSyntax }
class QsParseOptions { max_params: usize, max_depth: usize, allow_dots: bool, comma_arrays: bool, /* +3 fields */ }
class QsPathSegment { name: &[]u8, is_index: bool, index: usize, is_empty_bracket: bool, /* +3 fields */ }
class QsEntry { raw_key: &[]u8, decoded_key: &[]u8, decoded_value: &[]u8, is_null: bool, /* +8 fields */ }
class QsParsed { pool: [4096]u8, pool_len: usize, entries: [64]QsEntry, count: usize } // methods: init(...) +15
class QsBuilder { buf: []u8, pos: usize, count: usize, add_query_prefix: bool, /* +1 fields */ } // methods: init(...) +13
```

## API Signatures

```cp
fn isUnreserved(c: u8): bool;
fn needsDecode(s: &[]u8, plus_as_space: bool): bool;
fn parseDecimalIndex(s: &[]u8): usize?;
fn percentDecode(input: &[]u8, plus_as_space: bool, out: []u8): &[]u8 ! QsError;
fn percentEncode(input: &[]u8, space_as_plus: bool, out: []u8): &[]u8 ! QsError;
fn makeSegment(slice: &[]u8, base_off: u16, seg_rel_off: usize, in_pool: bool, is_empty_bracket: bool): QsPathSegment;
fn parseKeySegments(dec_key: &[]u8, key_off: u16, key_in_pool: bool, allow_dots: bool, max_depth: usize, out_segs: &mut [8]QsPathSegment): usize ! QsError;
```

## Minimal Usage

```cp
import { isUnreserved } from "qs";

pub fn main() {
    let result = isUnreserved(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"qs"`; install via `cap add qs`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
