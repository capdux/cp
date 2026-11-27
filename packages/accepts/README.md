[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# accepts

Zero-allocation RFC 9110 HTTP content negotiation engine for `Accept`, `Accept-Encoding`, `Accept-Charset`, and `Accept-Language` headers in pure CP, composing `negotiator`.

- **Upstream Alignment**: `npm` `accepts` (MIT, jshttp/accepts)
- **Package Version**: `1.0.0`
- **Memory Model**: Zero heap allocation — operates directly on `[]const u8` header slices and returns borrowed slices or candidate indices.

## Installation & Import

```bash
cap add accepts
```

```cp
import {
    AcceptHeaders,
    Accepts,
    parseQValue,
    negotiateEncoding,
    negotiateLanguage,
    negotiateMediaType,
    negotiateCharset,
    rankMediaTypes,
} from "accepts";
```

## Core API

| Symbol | Signature | Description |
| --- | --- | --- |
| `AcceptHeaders` | `class AcceptHeaders { accept, accept_encoding, accept_charset, accept_language: []const u8 }` | Request header bundle; `AcceptHeaders.empty()` creates an empty header set. |
| `Accepts` | `class Accepts { headers: AcceptHeaders }` | Stateful negotiator created via `Accepts.new(headers)`. |
| `Accepts.types` / `typeIndex` | `fn types(available: []const []const u8): ([]const u8)?` / `fn typeIndex(available): i64` | Selects the best matching media type (or its index, `-1` if none). |
| `Accepts.encodings` / `encodingIndex` | `fn encodings(available: []const []const u8): ([]const u8)?` / `fn encodingIndex(available): i64` | Selects the best matching content encoding (e.g. `br`, `gzip`, `identity`). |
| `Accepts.charsets` / `charsetIndex` | `fn charsets(available: []const []const u8): ([]const u8)?` / `fn charsetIndex(available): i64` | Selects the best matching character set (e.g. `utf-8`, `iso-8859-1`). |
| `Accepts.languages` / `languageIndex` | `fn languages(available: []const []const u8): ([]const u8)?` / `fn languageIndex(available): i64` | Selects the best matching BCP 47 language tag (e.g. `zh-CN`, `en-US`, `en`). |
| `parseQValue` | `fn parseQValue(s_in: []const u8): i32` | Parses an RFC 9110 quality weight (`0.000..1.000`) into fixed-point `0..1000` (`-1` on invalid input). |
| `negotiateMediaType` | `fn negotiateMediaType(accept_header: []const u8, available: []const []const u8): i64` | Standalone media-type negotiator returning best index or `-1`. |
| `negotiateEncoding` | `fn negotiateEncoding(accept_header: []const u8, available: []const []const u8): i64` | Standalone encoding negotiator returning best index or `-1`. |
| `negotiateCharset` | `fn negotiateCharset(accept_charset: []const u8, available: []const []const u8): i64` | Standalone charset negotiator returning best index or `-1`. |
| `negotiateLanguage` | `fn negotiateLanguage(accept_header: []const u8, available: []const []const u8): i64` | Standalone language negotiator returning best index or `-1`. |
| `rankMediaTypes` | `fn rankMediaTypes(accept_header: []const u8, available: []const []const u8, out_indices: []usize): usize` | Writes all acceptable candidate indices sorted by descending quality/specificity into `out_indices`; returns count written. |

## Examples

### 1. High-Level `Accepts` Negotiation

```cp
import { AcceptHeaders, Accepts } from "accepts";

fn negotiateRequest(): void {
    let mut h = AcceptHeaders.empty();
    h.accept = "text/html, application/json;q=0.9, */*;q=0.1";
    h.accept_encoding = "br, gzip;q=0.8";
    h.accept_language = "zh-CN, en;q=0.7";

    const acc = Accepts.new(h);
    const avail_types: [2][]const u8 = ["application/json", "text/html"];
    const best_type = acc.types(avail_types[0..]); // Some("text/html")

    const avail_enc: [2][]const u8 = ["gzip", "br"];
    const best_enc = acc.encodings(avail_enc[0..]); // Some("br")
}
```

### 2. Ranking Multiple Acceptable Media Types

```cp
import { rankMediaTypes } from "accepts";

fn rankDemo(): void {
    const avail: [3][]const u8 = ["text/plain", "application/json", "text/html"];
    let mut ranked: [3]usize = [0; 3];
    const count = rankMediaTypes("application/json;q=0.9, text/html;q=1.0", avail[0..], ranked[0..]);
    // count == 2; ranked[0] == 2 ("text/html"), ranked[1] == 1 ("application/json")
}
```

## Differences from Upstream & Highlights

- **Zero-Allocation Slice API**: Operates directly on `[]const u8` HTTP header slices with fixed-point q-values (`0..1000`), avoiding floating-point drift and JS array allocations.
- **Explicit Index & Optional Variants**: Provides both `*Index(): i64` (`-1` on mismatch) and `types`/`encodings`/`charsets`/`languages` returning `([]const u8)?` (`null` on mismatch).
- **Powered by `negotiator`**: Delegates RFC 9110 Accept-list parsing to `negotiator`.
