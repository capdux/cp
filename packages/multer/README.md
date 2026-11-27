[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# multer

Zero-heap pure CP multipart/form-data boundary extractor and upload policy validator (clean-room MIT implementation of npm multer v2)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:multer` ([https://github.com/expressjs/multer](https://github.com/expressjs/multer))

| Metadata | Value |
| --- | --- |
| Package | `multer` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add multer
```

```cp
import { MulterLimits, UploadedFile, MulterValidator, extractMultipartBoundary } from "multer";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MulterLimits` | `class` | Fields: `file_size: u64, files: usize, fields: usize, field_size: u64` · Methods: `defaults` |
| `UploadedFile` | `class` | Fields: `field_name: string, original_name: string, mime_type: string, size: u64` |
| `MulterValidator` | `class` | Fields: `limits: MulterLimits, files: [16]UploadedFile, file_count: usize, field_count: usize, allowed_fields: [16]string, allowed_count: usize` · Methods: `new, allowField, isFieldAllowed, validateContentType, acceptFile, acceptField, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `extractMultipartBoundary` | `(content_type: string): string?` | Invokes `extractMultipartBoundary` and returns `string?`. |

## Minimal Example

```cp
import { extractMultipartBoundary } from "multer";

pub fn main() {
    let result = extractMultipartBoundary("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
