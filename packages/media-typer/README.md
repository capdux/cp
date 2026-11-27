[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# media-typer

Zero-heap RFC 6838 HTTP media type parser, validator, classifier, and canonical lowercase formatter in pure CP.

- **Upstream Alignment**: Clean-room port of npm [`media-typer`](https://github.com/jshttp/media-typer) (`MIT`).
- **Package Version**: `1.0.0` (depends on `unicase`).

## Installation & Import

```bash
cap add media-typer
```

In `cap.json`:

```json
{
  "dependencies": {
    "media-typer": "^1.0.0"
  }
}
```

```typescript
import {
    MediaType, parse, parseMediaType, format, formatMediaType,
    testMediaType, testType, essence, mediaTypeEq, hasSuffix,
    isValidType, isValidSubtype, isValidSuffix,
    isStandardType, isVendorSubtype, isPersonalSubtype, isUnregisteredSubtype
} from "media-typer";
```

## Core API Reference

### `class MediaType`

Fields: `valid: bool`, `main_type: string`, `subtype: string`, `suffix: string`.

| Method | Signature | Description |
| :--- | :--- | :--- |
| `validate` | `MediaType.validate(raw: string): bool` | Static RFC 6838 syntax validation |
| `isValid` | `isValid(): bool` | Check whether the parsed `MediaType` is valid |
| `hasSuffix` | `hasSuffix(): bool` | Check whether `suffix` (the part after `+`) is non-empty |
| `formatTo` | `formatTo(out: []u8): usize` | Format as canonical lowercase `type/subtype[+suffix]` into `out` |

### Core Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `parse` / `parseMediaType` | `parse(raw: string): MediaType` | Parse `type/subtype[+suffix]` zero-copy (sets `valid = false` on invalid input) |
| `testMediaType` / `testType` | `testMediaType(raw: string): bool` | Validate raw RFC 6838 media type string |
| `format` | `format(mt: MediaType, out: []u8): usize` | Format `MediaType` to lowercase into `out`; returns `0` if invalid |
| `formatMediaType` | `formatMediaType(main_type: string, subtype: string, suffix: string, out: []u8): usize` | Validate components and format lowercase into `out` |
| `essence` | `essence(raw: string, out: []u8): usize` | Parse `raw` and write canonical lowercase essence into `out` |
| `mediaTypeEq` | `mediaTypeEq(a: string, b: string): bool` | Case-insensitive RFC 6838 equality check |
| `hasSuffix` | `hasSuffix(raw: string, suffix: string): bool` | Case-insensitive check if `raw` has structured suffix (e.g. `"json"`, `"xml"`) |
| `isValidType` / `isValidSubtype` / `isValidSuffix` | `isValidType(s: string): bool` | Component-level RFC 6838 token validation |
| `isStandardType` | `isStandardType(t: string): bool` | Check if top-level type is standard (`text`, `image`, `audio`, `video`, `application`, `multipart`, `message`, `font`, `model`, `example`) |
| `isVendorSubtype` / `isPersonalSubtype` / `isUnregisteredSubtype` | `isVendorSubtype(subtype: string): bool` | Detect `vnd.`, `prs.`, or `x.` subtype registration trees |

## Examples

### 1. Parsing & Inspecting Structured Suffixes

```typescript
import { parse, isVendorSubtype, hasSuffix } from "media-typer";

fn inspectHeader(raw: string): bool {
    let mt = parse(raw);
    if (!mt.isValid()) { return false; }
    // e.g. "application/vnd.api+json" -> main_type="application", subtype="vnd.api", suffix="json"
    return isVendorSubtype(mt.subtype) && hasSuffix(raw, "json");
}
```

### 2. Canonical Lowercase Formatting & Comparison

```typescript
import { parse, format, mediaTypeEq } from "media-typer";

fn canonicalize(raw: string, out: []u8): usize {
    let mt = parse(raw);
    let n = format(mt, out);
    // "IMAGE/SVG+XML" -> "image/svg+xml"
    let _ = mediaTypeEq(raw, "image/svg+xml"); // true
    return n;
}
```

## Differences from Upstream & CP Highlights

- **Zero-Copy Parsing**: `parse` returns slices into `raw` (`main_type`, `subtype`, `suffix`) without allocating or throwing exceptions—invalid inputs simply yield `mt.valid == false`.
- **Caller-Owned Formatting**: `format`, `formatMediaType`, `essence`, and `MediaType.formatTo` normalize to lowercase directly into `out: []u8`.
- **Rich Tree Classification**: Built-in `isStandardType`, `isVendorSubtype`, `isPersonalSubtype`, `isUnregisteredSubtype`, and `mediaTypeEq` powered by ASCII case folding from `unicase`.
