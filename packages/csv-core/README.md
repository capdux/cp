[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# csv-core

> **Source Attribution**: Clean-room MIT CP port of **crates.io** [`csv-core`](https://github.com/BurntSushi/rust-csv) (`v0.2` / `MIT`), CP package `csv-core@1.0.0`.

Zero-allocation streaming RFC 4180 CSV reader and writer state machine in pure CP.

## Why CP `csv-core`?

- **Zero-Heap Streaming DFA**: `CsvReader.readField` parses arbitrary input chunks into caller-supplied output buffers, returning `CSV_READ_INPUT_EMPTY` or `CSV_READ_OUTPUT_FULL` for seamless resumption without losing or duplicating bytes.
- **Zero-Overhead Stack Scratch**: Helper functions (`countCsvRecords`, `countCsvFieldsInLine`) use `undefined`-initialized `[128]u8` scratch buffers to avoid redundant stack zeroing.
- **Full RFC 4180 Compliance**: Handles quoted fields with embedded commas and newlines, doubled-quote escaping (`""` → `"`), CRLF (`\r\n`) and LF (`\n`) record endings, empty fields (`a,,b`), and custom delimiters (TSV `\t`, `;`, etc.).
- **Symmetric Encoder**: `CsvWriter` automatically quotes fields containing delimiters, quotes, or line breaks and doubles embedded quotes.

## Zero-Cost Migration (Rust → CP)

Keep the exact same package name (`csv-core`) — no renaming required.

**Rust (`Cargo.toml` / `main.rs`):**
```rust
use csv_core::{Reader, ReadFieldResult, Writer};
```

**CP (`cap.json` / `main.cp`):**
```typescript
import {
    CsvReader, CsvWriter, CsvReadResult,
    CSV_READ_INPUT_EMPTY, CSV_READ_OUTPUT_FULL, CSV_READ_FIELD,
    CSV_READ_RECORD_END, CSV_READ_END,
    countCsvRecords, countCsvFieldsInLine
} from "csv-core";
```

## Quick Start

### Stream-Parse CSV Fields into a Fixed Buffer

```typescript
import {
    CsvReader, CSV_READ_FIELD, CSV_READ_RECORD_END, CSV_READ_END
} from "csv-core";

export fn parseAllFields(data: &[]u8): usize {
    let mut reader = CsvReader.new();
    let mut out: [256]u8 = undefined;
    let mut pos: usize = 0;
    let mut fields: usize = 0;

    while (true) {
        let res = reader.readField(data[pos..data.len], out[0..256]);
        pos += res.bytes_read;
        if (res.status == CSV_READ_FIELD) {
            let field = out[0..res.bytes_written];
            fields += 1;
            _ = field;
        } else if (res.status == CSV_READ_RECORD_END) {
            // End of current row; reader.line() is updated
        } else if (res.status == CSV_READ_END) {
            break;
        } else if (pos >= data.len) {
            // Signal EOF by passing an empty slice
            let fin = reader.readField(data[data.len..data.len], out[0..256]);
            if (fin.status == CSV_READ_FIELD) { fields += 1; }
            if (fin.status == CSV_READ_END) { break; }
        }
    }
    return fields;
}
```

### Encode a CSV Record (or TSV with Custom Delimiter)

```typescript
import { CsvWriter } from "csv-core";

export fn writeRow(out: []u8): usize {
    let writer = CsvWriter.new();
    let cols: [3]string = ["alice", "hello, \"world\"", "42"];
    return writer.writeRecord(cols[0..3], out);
}
```

## API Reference

- **Status Constants (`i32`)**: `CSV_READ_INPUT_EMPTY (0)`, `CSV_READ_OUTPUT_FULL (1)`, `CSV_READ_FIELD (2)`, `CSV_READ_RECORD_END (3)`, `CSV_READ_END (4)`.
- **`CsvReadResult`**: `{ status: i32, bytes_read: usize, bytes_written: usize }`.
- **`CsvReader`**: `new()`, `withDelimiter(delim: u8)`, `readField(input: &[]u8, out: []u8): CsvReadResult`, `line(): u64`, `reset()`.
- **`CsvWriter`**: `new()`, `withDelimiter(delim: u8)`, `writeField(field: &[]u8, out: []u8): usize`, `writeRecord(fields: &[]string, out: []u8): usize`.
- **Helpers**: `countCsvRecords(input: &[]u8): usize`, `countCsvFieldsInLine(input: &[]u8): usize`.

## License

MIT
