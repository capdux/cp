# object-inspect — AI Agent Quick Ref

Zero-heap debug value inspector and string/bytes/number pretty-formatter with escape and truncation support (`v1.13.4`, `MIT`)

## Import

```cp
import {
    InspectOptions,
    defaultInspectOptions,
    inspectString,
    inspectBool,
    inspectInt,
    inspectBytes,
    inspectStringArray,
} from "object-inspect";
```

## Core Types & Constants

```cp
class InspectOptions { quote_style_double: bool, max_string_len: usize }
```

## API Signatures

```cp
fn defaultInspectOptions(): InspectOptions;
fn inspectString(s: string, opts: InspectOptions, out: []u8): usize;
fn inspectBool(v: bool, out: []u8): usize;
fn inspectInt(val: i64, out: []u8): usize;
fn inspectBytes(data: &[]u8, max_bytes: usize, out: []u8): usize;
fn inspectStringArray(items: &[]string, opts: InspectOptions, out: []u8): usize;
```

## Minimal Usage

```cp
import { inspectString } from "object-inspect";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = inspectString("example", /* opts: InspectOptions */, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"object-inspect"`; install via `cap add object-inspect`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
