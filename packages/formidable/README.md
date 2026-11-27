[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# formidable

Zero-heap pure CP multipart/form-data parser for fields and file uploads (clean-room MIT implementation of npm formidable v3)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:formidable` ([https://github.com/node-formidable/formidable](https://github.com/node-formidable/formidable))

| Metadata | Value |
| --- | --- |
| Package | `formidable` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add formidable
```

```cp
import {
    FormidableOptions,
    FormField,
    FormFile,
    ParsedForm,
    frmIsOws,
    frmIsTokenChar,
    frmTrimOws,
    frmSkipOws,
} from "formidable";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FormidableOptions` | `class` | Fields: `max_files: usize, max_fields: usize, max_file_size: u64, allow_empty_files: bool` · Methods: `defaults` |
| `FormField` | `class` | Fields: `name: string, value: string` |
| `FormFile` | `class` | Fields: `field_name: string, original_filename: string, mime_type: string, body_start: usize, body_len: usize` |
| `ParsedForm` | `class` | Fields: `fields: [16]FormField, field_count: usize, files: [16]FormFile, file_count: usize, valid: bool, err_code: u8` · Methods: `empty, fail, getField, getFile` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `frmIsOws` | `(b: u8): bool` | Invokes `frmIsOws` and returns `bool`. |
| `frmIsTokenChar` | `(b: u8): bool` | Invokes `frmIsTokenChar` and returns `bool`. |
| `frmTrimOws` | `(s: string): string` | Invokes `frmTrimOws` and returns `string`. |
| `frmSkipOws` | `(s: string, start: usize): usize` | Invokes `frmSkipOws` and returns `usize`. |
| `frmIsBoundaryTerminator` | `(body: string, after_pos: usize): bool` | Invokes `frmIsBoundaryTerminator` and returns `bool`. |
| `frmFindFirstBoundary` | `(body: string, boundary: string): usize` | Invokes `frmFindFirstBoundary` and returns `usize`. |
| `frmFindNextBoundary` | `(body: string, start: usize, boundary: string): usize` | Invokes `frmFindNextBoundary` and returns `usize`. |
| `frmExtractDispositionName` | `(header: string): string` | Invokes `frmExtractDispositionName` and returns `string`. |
| `frmHasFilenameParam` | `(header: string): bool` | Invokes `frmHasFilenameParam` and returns `bool`. |
| `parseMultipartForm` | `(content_type: string, body: string, opts: FormidableOptions): ParsedForm` | Parses input via `parseMultipartForm` and returns `ParsedForm`. |

## Minimal Example

```cp
import { parseMultipartForm } from "formidable";

pub fn main() {
    let result = parseMultipartForm("example", "example", /* opts: FormidableOptions */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
