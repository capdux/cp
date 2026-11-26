# @cap/csv-line

Zero-heap RFC 4180 CSV/TSV record parser and serializer in pure CP (npm `csv-parse` / `papaparse` parity).

## Features

- `parseRecord(src, start_pos, delim, out_buf, out_spans)`: parses one RFC 4180 record at a time with quoted fields, `""` unescaping, embedded newlines/delimiters, CRLF/LF line endings, and `next_pos` streaming iteration
- `formatField(field, delim, dst)` & `formatRecord(fields, delim, dst)`: RFC 4180 quoting and `""` escaping
- `formatFieldSafe(field, delim, dst)`: optional spreadsheet formula prefix guard (`=`, `+`, `-`, `@`)

## Usage

```cp
import { parseRecord, formatRecord, CsvFieldSpan } from "@cap/csv-line";

let out_buf: [256]u8 = undefined;
let spans: [16]CsvFieldSpan = undefined;
const r = parseRecord("a,\"b,c\",d\n", 0, ',' as u8, out_buf[0..256], spans[0..16]);
```
