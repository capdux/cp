# @cap/csv-core

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:csv-core` ([https://github.com/BurntSushi/rust-csv](https://github.com/BurntSushi/rust-csv))

Pure CP, zero-allocation streaming RFC 4180 CSV reader and writer state machine (clean-room MIT parity with Rust `csv-core` v0.2).

## Design

- **Zero-Heap Streaming DFA**: `CsvReader.readField` operates on arbitrary input chunks and caller-supplied output buffers, returning `CSV_READ_INPUT_EMPTY` or `CSV_READ_OUTPUT_FULL` for seamless resumption without losing or duplicating bytes.
- **Full RFC 4180 Compliance**: Properly handles quoted fields containing commas and embedded newlines, doubled-quote escaping (`""` -> `"`), CRLF (`\r\n`) and LF (`\n`) record endings, empty fields (`a,,b`), and custom delimiters (TSV `\t`, `;`, etc.).
- **Symmetric Encoder**: `CsvWriter` automatically quotes fields containing delimiters, quotes, or newlines and escapes embedded quotes.

## API

### Constants & Result
- `CSV_READ_INPUT_EMPTY = 0`, `CSV_READ_OUTPUT_FULL = 1`, `CSV_READ_FIELD = 2`, `CSV_READ_RECORD_END = 3`, `CSV_READ_END = 4`
- `CsvReadResult { status: i32, bytes_read: usize, bytes_written: usize }`

### `CsvReader`
- `CsvReader.new()` / `CsvReader.withDelimiter(delim: u8)`
- `readField(*_, input: []const u8, out: []u8): CsvReadResult`
- `line(): u64`, `reset(*_)`

### `CsvWriter`
- `CsvWriter.new()` / `CsvWriter.withDelimiter(delim: u8)`
- `writeField(field: []const u8, out: []u8): usize`
- `writeRecord(fields: []const string, out: []u8): usize`

### Helpers
- `countCsvRecords(input: []const u8): usize`
- `countCsvFieldsInLine(input: []const u8): usize`

## License

MIT
