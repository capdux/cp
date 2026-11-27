# csv-parse — AI Agent Quick Ref

Zero-heap RFC 4180 CSV/TSV record parser and serializer with quoted fields, escaped quotes, embedded newlines, custom delimiters, and spreadsheet formula guard. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CsvFieldSpan,
    CsvParseResult,
    parseRecord,
    formatField,
    formatFieldSafe,
    formatRecord,
} from "csv-parse";
```

## Core Types & Constants

```cp
class CsvFieldSpan { start: usize, len: usize }
class CsvParseResult { field_count: usize, next_pos: usize, ok: bool }
```

## API Signatures

```cp
fn parseRecord(src: &[]u8, start_pos: usize, delim: u8, out_buf: []u8, out_spans: []CsvFieldSpan): CsvParseResult;
fn formatField(field: &[]u8, delim: u8, dst: []u8): i64;
fn formatFieldSafe(field: &[]u8, delim: u8, dst: []u8): i64;
fn formatRecord(fields: &[]&[]u8, delim: u8, dst: []u8): i64;
```

## Minimal Usage

```cp
import { formatField } from "csv-parse";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = formatField(out_buf[0..256], 0, buf_1[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"csv-parse"`; install via `cap add csv-parse`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
