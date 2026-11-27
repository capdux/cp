# querystringify — AI Agent Quick Ref

Zero-allocation WHATWG application/x-www-form-urlencoded parser, serializer, and percent-encoding codec for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    FormPair,
    FormSerializer,
    isFormSafe,
    encodedSize,
    percentEncode,
    decodedSize,
    percentDecode,
    parseFormPairs,
} from "querystringify";
```

## Core Types & Constants

```cp
class FormPair { key_start: usize, key_len: usize, val_start: usize, val_len: usize }
class FormSerializer { len: usize, count: usize } // methods: new(...) +1
```

## API Signatures

```cp
fn isFormSafe(b: u8): bool;
fn encodedSize(input: string, space_as_plus: bool): usize;
fn percentEncode(input: string, space_as_plus: bool, out: []u8): usize;
fn decodedSize(input: string): usize;
fn percentDecode(input: string, plus_as_space: bool, out: []u8): usize;
fn parseFormPairs(query: string, out: []FormPair): usize;
fn decodedMatches(raw: string, plus_as_space: bool, target: string): bool;
fn getParamDecoded(query: string, target_key: string, out: []u8): i64;
```

## Minimal Usage

```cp
import { encodedSize } from "querystringify";

pub fn main() {
    let result = encodedSize("example", false);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"querystringify"`; install via `cap add querystringify`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
