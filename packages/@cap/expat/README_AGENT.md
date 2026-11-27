# @cap/expat — AI Agent Quick Ref

Stream-oriented XML 1.0 SAX parser (libexpat 2.6.4) native C binding for CP (`v2.6.4`, `MIT`)

## Import

```cp
import {
    ExpatError,
    cp_expat_version,
    cp_expat_count_elements,
    versionString,
    countElements,
} from "@cap/expat";
```

## Core Types & Constants

```cp
error ExpatError { InvalidInput, ParserInitFailed, ParseFailed }
```

## API Signatures

```cp
fn cp_expat_version(): [*:0]&u8!;
fn cp_expat_count_elements(xml: [*]&u8!?, len: usize, out_elem_count: *usize!): i32;
fn versionString(): string;
fn countElements(xml: &[]u8): !usize;
```

## Minimal Usage

```cp
import { versionString } from "@cap/expat";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/expat"`; install via `cap add @cap/expat`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
