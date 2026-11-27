[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# type-is

Zero-heap HTTP `Content-Type` matcher for CP, modeled on npm [`type-is`](https://github.com/jshttp/type-is) (MIT, CP v1.0.0, built on `mime-types` and `unicase`).

Supports extension shorthands (`"json"`, `"html"`, `"urlencoded"`, `"multipart"`), MIME wildcards (`"text/*"`, `"*/json"`, `"*/*"`), structured syntax suffixes (`"+json"`, `"+xml"`), and HTTP request body detection (`hasBody` / `requestHasBody` / `typeis`).

## Installation & Import

```bash
cap add type-is
```

```cp
import {
    RequestHeaders,
    is,
    typeIs,
    typeis,
    hasBody,
    normalize,
    isJson,
} from "type-is";
```

## Core API Reference

### Data Structures

| Type | Fields | Description |
| :--- | :--- | :--- |
| `RequestHeaders` | `content_type: string`, `transfer_encoding: string`, `content_length: string` | Minimal HTTP header tuple used by `requestHasBody`, `typeis`, and `typeisCsv`. |

### Matching & Normalization Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `is` | `(content_type: string, types: &[]string): string?` | Matches `content_type` against candidate patterns in order; returns the first matching pattern (or normalized essence if `types` is empty), or `null`. |
| `typeIs` | `(content_type: string, candidates_csv: string): string?` | Same as `is`, taking a comma-separated list of candidate patterns (e.g. `"json, +json, text/*"`). |
| `typeis` | `(req: RequestHeaders, types: &[]string): string?` | Checks `requestHasBody(req)` first; returns `null` if the request has no body, otherwise matches `req.content_type`. |
| `typeisCsv` | `(req: RequestHeaders, candidates_csv: string): string?` | Request-level CSV matcher with `requestHasBody` guard. |
| `hasBody` | `(transfer_encoding: string, content_length: string): bool` | Returns `true` if `transfer_encoding` is non-empty or `content_length` is non-empty. |
| `requestHasBody` | `(req: RequestHeaders): bool` | Convenience wrapper over `hasBody` for `RequestHeaders`. |
| `typeMatches` | `(content_type: string, pattern: string): bool` | Tests whether a single `Content-Type` header matches `pattern` (shorthand, wildcard, or `+suffix`). |
| `matchType` | `(expected: string, actual: string): bool` | Matches a normalized expected pattern against a normalized actual MIME essence. |
| `normalize` | `(pattern: string): string?` | Strict npm `type-is.normalize` equivalent; returns `null` for empty strings or unknown extensions. |
| `normalizeTypePattern` | `(pattern: string): string` | Lenient normalization returning `""` on empty input or the trimmed pattern if unknown. |
| `extractEssence` | `(content_type: string): string?` | Extracts `type/subtype` essence while stripping parameters such as `; charset=utf-8`. |
| `isValidContentType` | `(content_type: string): bool` | Returns `true` if `extractEssence` succeeds. |
| `isJson` / `isText` / `isForm` / `isMultipart` | `(content_type: string): bool` | Fast boolean helpers for common HTTP body media types. |

## Usage Examples

### 1. Match Content-Type Against Candidate Slice (`is`)

```cp
import { is } from "type-is";

fn routeBody(ct: string): string {
    let candidates = ["urlencoded", "json", "+json", "text/*"];
    let matched = is(ct, &candidates);
    if (matched != null) {
        return matched.?;
    }
    return "unsupported";
}
// routeBody("application/vnd.api+json; charset=utf-8") -> "+json"
```

### 2. Full Request Body & Type Guard (`typeis`)

```cp
import { RequestHeaders, typeis } from "type-is";

fn handleRequest(req: RequestHeaders): bool {
    let allowed = ["json", "application/*+json"];
    let matched = typeis(req, &allowed);
    // Returns null immediately if neither Content-Length nor Transfer-Encoding is set
    return matched != null;
}
```

### 3. CSV Pattern & Quick Helpers

```cp
import { typeIs, isJson, isMultipart, normalize } from "type-is";

fn quickChecks() {
    let m = typeIs("text/html; charset=utf-8", "json, html, text/*"); // "html"
    let ok = isJson("application/problem+json"); // true
    let mp = isMultipart("multipart/form-data; boundary=abc"); // true
    let norm = normalize("urlencoded"); // "application/x-www-form-urlencoded"
}
```

## Differences from Upstream & Zero-Copy Highlights

- **Zero Heap Allocation**: All parsing, trimming, parameter stripping, and case-insensitive matching operate directly on borrowed `string` slices via `cap:ascii`, `mime-types`, and `unicase`.
- **Optional Return Semantics**: Uses CP optional `string?` (`null` on no match or no body) instead of JavaScript's mixed `string | false | null` return union.
- **Structured `RequestHeaders`**: Replaces Node.js `IncomingMessage` objects with a lightweight 3-field value struct.
