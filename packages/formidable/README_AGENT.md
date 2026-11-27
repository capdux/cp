# formidable — AI Agent Quick Ref

Zero-heap pure CP multipart/form-data parser for fields and file uploads (clean-room MIT implementation of npm formidable v3) (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class FormidableOptions { max_files: usize, max_fields: usize, max_file_size: u64, allow_empty_files: bool } // methods: defaults(...)
class FormField { name: string, value: string }
class FormFile { field_name: string, original_filename: string, mime_type: string, body_start: usize, /* +1 fields */ }
class ParsedForm { fields: [16]FormField, field_count: usize, files: [16]FormFile, file_count: usize, /* +2 fields */ } // methods: empty(...) +3
```

## API Signatures

```cp
fn frmIsOws(b: u8): bool;
fn frmIsTokenChar(b: u8): bool;
fn frmTrimOws(s: string): string;
fn frmSkipOws(s: string, start: usize): usize;
fn frmIsBoundaryTerminator(body: string, after_pos: usize): bool;
fn frmFindFirstBoundary(body: string, boundary: string): usize;
fn frmFindNextBoundary(body: string, start: usize, boundary: string): usize;
fn frmExtractDispositionName(header: string): string;
fn frmHasFilenameParam(header: string): bool;
fn parseMultipartForm(content_type: string, body: string, opts: FormidableOptions): ParsedForm;
```

## Minimal Usage

```cp
import { parseMultipartForm } from "formidable";

pub fn main() {
    let result = parseMultipartForm("example", "example", /* opts: FormidableOptions */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"formidable"`; install via `cap add formidable`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
