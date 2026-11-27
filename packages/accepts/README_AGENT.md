# accepts — AI Agent Quick Ref

Zero-allocation RFC 9110 HTTP content negotiation for `Accept`, `Accept-Encoding`, `Accept-Charset`, and `Accept-Language` (npm `accepts` parity).

## Import

```cp
import {
    AcceptHeaders, Accepts, parseQValue,
    negotiateEncoding, negotiateLanguage, negotiateMediaType,
    negotiateCharset, rankMediaTypes,
} from "accepts";
```

## API Signatures

```cp
export class AcceptHeaders {
    accept: []const u8, accept_encoding: []const u8,
    accept_charset: []const u8, accept_language: []const u8,
    static fn empty(): AcceptHeaders;
}
export class Accepts {
    headers: AcceptHeaders,
    static fn new(headers: AcceptHeaders): Accepts;
    fn typeIndex(available: []const []const u8): i64;
    fn types(available: []const []const u8): ([]const u8)?;
    fn encodingIndex(available: []const []const u8): i64;
    fn encodings(available: []const []const u8): ([]const u8)?;
    fn charsetIndex(available: []const []const u8): i64;
    fn charsets(available: []const []const u8): ([]const u8)?;
    fn languageIndex(available: []const []const u8): i64;
    fn languages(available: []const []const u8): ([]const u8)?;
}
export fn parseQValue(s_in: []const u8): i32;
export fn negotiateEncoding(accept_header: []const u8, available: []const []const u8): i64;
export fn negotiateLanguage(accept_header: []const u8, available: []const []const u8): i64;
export fn negotiateMediaType(accept_header: []const u8, available: []const []const u8): i64;
export fn negotiateCharset(accept_charset: []const u8, available: []const []const u8): i64;
export fn rankMediaTypes(accept_header: []const u8, available: []const []const u8, out_indices: []usize): usize;
```

## Minimal Example

```cp
import { AcceptHeaders, Accepts } from "accepts";

fn demo(): void {
    let mut h = AcceptHeaders.empty();
    h.accept = "application/json, text/plain;q=0.5";
    const acc = Accepts.new(h);
    const avail: [2][]const u8 = ["text/plain", "application/json"];
    const best = acc.types(avail[0..]); // Some("application/json")
}
```

## Caveats & Borrowing Rules

- **Byte slices**: All header fields and `available` elements are `[]const u8` (not owned strings); returned `([]const u8)?` borrows directly from the `available` slice passed in.
- **Mismatch sentinels**: `*Index` and `negotiate*` functions return `-1` when no candidate is acceptable (`q=0` or unmatched); `types`/`encodings`/`charsets`/`languages` return `null`.
- **Fixed-point q-values**: `parseQValue` returns `0..1000` (e.g. `"0.75"` -> `750`), or `-1` for malformed values.
- **Mutable output**: `rankMediaTypes` requires a mutable `out_indices: []usize` slice and writes at most `out_indices.len` entries.
