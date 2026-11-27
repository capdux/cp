# `mime-types` — AI Agent Quick Ref

Zero-heap MIME lookup, extension reverse lookup, charset, `Content-Type` builder & compressibility check (npm `mime-types` v2.1 parity).

## Import

```cp
import {
    MimeParts,
    parseMimeType,
    lookup,
    extension,
    charset,
    contentType,
    isCompressible,
} from "mime-types";
```

## Core API Signatures

```cp
struct MimeParts {
    valid: bool,
    essence: string,      // e.g. "text/html"
    type_part: string,    // e.g. "text"
    subtype: string,      // e.g. "html"
    charset_val: string,  // e.g. "utf-8" or ""
}

fn parseMimeType(raw: string): MimeParts;
fn lookup(path_or_ext: string): string?;         // null on miss
fn extension(mime_type: string): string?;        // no leading '.', null on miss
fn charset(mime_type: string): string?;          // e.g. "UTF-8" or null
fn contentType(type_or_ext: string, out: []u8): usize; // 0 if unknown
fn isCompressible(mime_type: string): bool;
```

## Minimal Example

```cp
import { lookup, contentType, isCompressible } from "mime-types";

const mime = lookup("data.json"); // "application/json"
if (mime != null && isCompressible(mime.?)) {
    let hdr: [128]u8 = undefined;
    const n = contentType("data.json", hdr[0..128]);
    // hdr[0..n] == "application/json; charset=utf-8"
}
```

## Gotchas & Borrow Rules

- `lookup`, `extension`, and `charset` return CP optional `string?` (not `false`); always unwrap with `if (x != null) { x.? }`.
- `parseMimeType` returns zero-copy slices into `raw`; keep `raw` alive while reading `MimeParts` fields.
- `contentType` returns `0` when the input cannot be resolved; check `n > 0` before using `out[0..n]`.
- Case-insensitive matching is handled internally for extensions and MIME types.
