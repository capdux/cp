[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# `mime-types`

Zero-heap MIME type, file extension, charset, `Content-Type` header builder, and compressibility resolver in pure CP — a clean-room MIT counterpart to npm [`mime-types`](https://github.com/jshttp/mime-types) (v2.1 parity).

## Installation & Import

```bash
cap add mime-types
```

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

## Core API

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `MimeParts` | `struct { valid: bool, essence: string, type_part: string, subtype: string, charset_val: string }` | Zero-copy parsed MIME components slicing the input string |
| `parseMimeType` | `(raw: string): MimeParts` | Parse `"text/html; charset=utf-8"` into `type_part`, `subtype`, `essence`, `charset_val` |
| `lookup` | `(path_or_ext: string): string?` | Resolve file path, filename, or extension (e.g., `"app.wasm"`, `".json"`, `"png"`) to MIME type, or `null` |
| `extension` | `(mime_type: string): string?` | Return default file extension (without leading dot) for a MIME type, or `null` |
| `charset` | `(mime_type: string): string?` | Return default charset (e.g., `"UTF-8"`) for text/JSON/XML/JS/SVG types, or `null` |
| `contentType` | `(type_or_ext: string, out: []u8): usize` | Build full `Content-Type` header (appending `; charset=utf-8` when applicable) into `out`; returns bytes written (`0` if unknown) |
| `isCompressible` | `(mime_type: string): bool` | Return `true` if the MIME type benefits from gzip/brotli/zstd compression |

## Examples

### 1. Resolving `Content-Type` and Compression Eligibility for HTTP Responses

```cp
import { print } from "cap:io";
import { lookup, contentType, isCompressible } from "mime-types";

fn main() {
    const mime = lookup("/static/bundle.js");
    if (mime != null) {
        print("mime:", mime.?);                    // "text/javascript"
        print("compress:", isCompressible(mime.?)); // true
    }

    let hdr: [128]u8 = undefined;
    const n = contentType("index.html", hdr[0..128]);
    if (n > 0) {
        print("Content-Type:", hdr[0..n]);         // "text/html; charset=utf-8"
    }
}
```

### 2. Parsing Incoming MIME Headers & Reverse Extension Lookup

```cp
import { print } from "cap:io";
import { parseMimeType, extension, charset } from "mime-types";

fn main() {
    const parts = parseMimeType("application/json; charset=utf-8");
    if (parts.valid) {
        print("essence:", parts.essence);     // "application/json"
        print("type:", parts.type_part);      // "application"
        print("subtype:", parts.subtype);     // "json"
        print("charset:", parts.charset_val); // "utf-8"
    }

    const ext = extension("image/webp");
    if (ext != null) { print("ext:", ext.?); } // "webp"

    const cs = charset("text/markdown");
    if (cs != null) { print("default charset:", cs.?); } // "UTF-8"
}
```

## Differences from Upstream & Performance Highlights

- **Zero Heap Allocation**: All lookup tables are compiled into static read-only data; `lookup`, `extension`, and `charset` return optional static slices (`string?`), while `contentType` writes directly into a caller-owned `[]u8` buffer.
- **Typed Optional Returns**: Uses CP `string?` (`null` on miss) instead of JavaScript's `string | false` union.
- **Built-in Compressibility Table**: Includes `isCompressible()` (matching `compressible` / `mime-db`) for `+json`, `+xml`, `+text`, `text/*`, `application/wasm`, `image/svg+xml`, and more.
