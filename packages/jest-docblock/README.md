[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# jest-docblock

Zero-heap top-of-file docblock pragma extractor, stripper, parser, and formatter in pure CP.

- **Upstream Alignment**: Modeled after npm [`jest-docblock`](https://github.com/jestjs/jest) `29.7.0` (`MIT`).
- **Package Version**: `29.7.0`

## Installation & Import

```bash
cap add jest-docblock
```

In `cap.json`:

```json
{
  "dependencies": {
    "jest-docblock": "^29.7.0"
  }
}
```

```typescript
import {
    DocPragma, ParsedDocblock,
    extract, extractDocblock, strip, stripSlice,
    parse, parseDocblock, parseWithComments,
    print, printDocblock, printWithComments,
    getPragma, hasPragma
} from "jest-docblock";
```

## Core API Reference

### Data Structures

| Type | Fields | Description |
| :--- | :--- | :--- |
| `DocPragma` | `key: string`, `value: string` | Single `@key value` pragma (slices borrowed from source docblock) |
| `ParsedDocblock` | `comments_len: usize`, `pragma_count: usize` | Counts returned by `parseWithComments` |

### Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `extract` / `extractDocblock` | `extract(code: string): string` | Extract leading `/** ... */` docblock slice from source code (zero-copy) |
| `strip` | `strip(code: string, out: []u8): usize` | Copy `code` without its leading docblock into `out`; returns bytes written |
| `stripSlice` | `stripSlice(code: string): string` | Return zero-copy slice of `code` after stripping leading docblock |
| `parse` / `parseDocblock` | `parse(docblock: string, out: []DocPragma): usize` | Parse `@key value` pragmas into `out`; returns pragma count |
| `parseWithComments` | `parseWithComments(docblock: string, comments_out: []u8, pragmas_out: []DocPragma): ParsedDocblock` | Separate free-form comments (written to `comments_out`) and pragmas (written to `pragmas_out`) |
| `print` / `printDocblock` | `print(pragmas: &[]DocPragma, out: []u8): usize` | Format pragmas as a `/** ... */` docblock into `out` |
| `printWithComments` | `printWithComments(comments: string, pragmas: &[]DocPragma, out: []u8): usize` | Format leading comments and pragmas into a `/** ... */` docblock |
| `getPragma` | `getPragma(pragmas: &[]DocPragma, key: string): string?` | Look up first pragma value matching `key`, or `null` |
| `hasPragma` | `hasPragma(pragmas: &[]DocPragma, key: string): bool` | Check whether `key` exists in `pragmas` |

## Examples

### 1. Extracting & Parsing `@jest-environment` Pragmas

```typescript
import { extract, parse, getPragma, hasPragma, DocPragma } from "jest-docblock";

fn detectEnv(src: string): string {
    let doc = extract(src);
    let mut pragmas: [16]DocPragma = undefined;
    let count = parse(doc, &mut pragmas);
    let slice = pragmas[0..count];

    if (hasPragma(slice, "jest-environment")) {
        return getPragma(slice, "jest-environment") orelse "node";
    }
    return "node";
}
```

### 2. Round-Trip Parsing with Comments & Formatting

```typescript
import { parseWithComments, printWithComments, DocPragma } from "jest-docblock";

fn reformatDocblock(doc: string, out: []u8): usize {
    let mut comments_buf: [256]u8 = undefined;
    let mut pragmas: [16]DocPragma = undefined;
    let parsed = parseWithComments(doc, &mut comments_buf, &mut pragmas);

    let comments = comments_buf[0..parsed.comments_len] as string;
    return printWithComments(comments, pragmas[0..parsed.pragma_count], out);
}
```

## Differences from Upstream & CP Highlights

- **Zero-Copy Extraction & Stripping**: `extract` and `stripSlice` return direct sub-slices of `code` with no allocation.
- **Caller-Owned Output Buffers**: `parse`, `parseWithComments`, `strip`, `print`, and `printWithComments` write into caller-supplied slices (`[]DocPragma` / `[]u8`) and return written counts.
- **Explicit Optional Lookups**: `getPragma` returns `string?` instead of `undefined`, enforcing null-safe handling via `orelse` or `if (v := ...)`.
