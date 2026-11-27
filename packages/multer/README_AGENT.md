# multer — AI Agent Quick Ref

Zero-heap pure CP multipart/form-data boundary extractor and upload policy validator (clean-room MIT implementation of npm multer v2) (`v1.0.0`, `MIT`)

## Import

```cp
import { MulterLimits, UploadedFile, MulterValidator, extractMultipartBoundary } from "multer";
```

## Core Types & Constants

```cp
class MulterLimits { file_size: u64, files: usize, fields: usize, field_size: u64 } // methods: defaults(...)
class UploadedFile { field_name: string, original_name: string, mime_type: string, size: u64 }
class MulterValidator { limits: MulterLimits, files: [16]UploadedFile, file_count: usize, field_count: usize, /* +2 fields */ } // methods: new(...) +7
```

## API Signatures

```cp
fn extractMultipartBoundary(content_type: string): string?;
```

## Minimal Usage

```cp
import { extractMultipartBoundary } from "multer";

pub fn main() {
    let result = extractMultipartBoundary("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"multer"`; install via `cap add multer`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
