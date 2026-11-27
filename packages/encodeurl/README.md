[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# encodeurl

Zero-heap idempotent URL encoder, normalized percent-encoder, decoder, and validator that percent-encodes unsafe characters without double-encoding existing `%XX` sequences in pure CP.

- **Package**: `encodeurl` (`v1.0.0`)
- **Upstream Alignment**: Clean-room CP port of `npm/encodeurl` (`MIT`, `pillarjs/encodeurl`), extended with checked buffer sizing, `%xx` uppercase normalization, and URL decoding.

## Installation & Import

```bash
cap add encodeurl
```

```typescript
import {
    needsUrlEncode, encodedUrlLen,
    encodeUrl, encodeUrlChecked, encodeUrlNormalized,
    decodeUrl, isEncodedUrl,
} from "encodeurl";
```

## Core API

| Function | Signature | Description |
| --- | --- | --- |
| `needsUrlEncode` | `(b: u8): bool` | Returns `true` if byte `b` must be percent-encoded (control chars, space, non-ASCII, `"`, `<`, `>`, `\`, `^`, `` ` ``, `{`, `\|`, `}`). |
| `encodedUrlLen` | `(url: string): usize` | Computes exact output byte length for `encodeUrl(url, out)`. |
| `encodeUrl` | `(url: string, out: []u8): usize` | Idempotently percent-encodes `url` into `out` while preserving valid `%XX` sequences; returns bytes written. |
| `encodeUrlChecked` | `(url: string, out: []u8): usize?` | Same as `encodeUrl`, but returns `null` if `out.len < encodedUrlLen(url)`. |
| `encodeUrlNormalized` | `(url: string, out: []u8): usize` | Percent-encodes `url` and uppercases hex digits in existing `%xx` sequences (e.g. `%2f` -> `%2F`). |
| `decodeUrl` | `(url: string, out: []u8): usize` | Decodes valid `%XX` sequences back to raw bytes into `out`, leaving invalid `%` literals intact. |
| `isEncodedUrl` | `(url: string): bool` | Returns `true` if `url` contains no characters requiring encoding and no malformed `%` escapes. |

## Examples

### 1. Idempotent URL Encoding (Safe for `Location` Headers)

```typescript
import { encodeUrl, encodedUrlLen, isEncodedUrl } from "encodeurl";

fn main() {
    let raw = "/files/my doc%20(1).pdf?q=a|b";
    let buf: [128]u8 = undefined;
    let n = encodeUrl(raw, buf[0..]);
    // buf[0..n] == "/files/my%20doc%20(1).pdf?q=a%7Cb" (existing %20 not double-encoded)
    let ok = isEncodedUrl(buf[0..n]); // true
}
```

### 2. Buffer-Checked Encoding & Hex Normalization

```typescript
import { encodeUrlChecked, encodeUrlNormalized } from "encodeurl";

fn main() {
    let out: [64]u8 = undefined;
    if (let n = encodeUrlChecked("/hello world", out[0..])) {
        // out[0..n] == "/hello%20world"
    }

    let norm: [64]u8 = undefined;
    let m = encodeUrlNormalized("/a%2fb c", norm[0..]);
    // norm[0..m] == "/a%2Fb%20c"
}
```

### 3. Percent-Decoding

```typescript
import { decodeUrl } from "encodeurl";

fn main() {
    let out: [64]u8 = undefined;
    let n = decodeUrl("/hello%20world%21", out[0..]);
    // out[0..n] == "/hello world!"
}
```

## Differences from Upstream & Highlights

- **Zero Heap Allocation**: All functions write into caller-supplied `out: []u8` slices, making them safe for HTTP server hot paths.
- **No Double-Encoding**: Preserves valid `%XX` sequences identically to `pillarjs/encodeurl`, while encoding stray `%` signs as `%25`.
- **Extended Toolkit**: Adds `encodeUrlChecked` (returns `usize?`), `encodeUrlNormalized`, `decodeUrl`, `encodedUrlLen`, and `isEncodedUrl`.
