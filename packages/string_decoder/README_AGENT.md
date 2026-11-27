# string_decoder — AI Agent Quick Ref

Zero-allocation streaming multi-byte UTF-8/UTF-16/Latin1/Base64 string decoder preserving incomplete boundary bytes across chunks (clean-room MIT implementation of npm string_decoder, #62 / 378M wk) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    DecoderEncoding,
    StringDecoder,
    sdecUtf8ExpectedLen,
    sdecPushReplacement,
    sdecPushCodePointUtf8,
} from "string_decoder";
```

## Core Types & Constants

```cp
enum DecoderEncoding { Utf8, Latin1, Utf16Le }
class StringDecoder { encoding: DecoderEncoding, last_char: [4]u8, last_need: usize, last_total: usize, /* +1 fields */ } // methods: new(...) +6
```

## API Signatures

```cp
fn sdecUtf8ExpectedLen(lead: u8): usize;
fn sdecPushReplacement(out: []u8, pos: usize): usize;
fn sdecPushCodePointUtf8(cp: u32, out: []u8, pos: usize): usize;
```

## Minimal Usage

```cp
import { sdecUtf8ExpectedLen } from "string_decoder";

pub fn main() {
    let result = sdecUtf8ExpectedLen(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"string_decoder"`; install via `cap add string_decoder`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
