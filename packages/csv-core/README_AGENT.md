# csv-core (AI Agent Quick Ref)

- **Import**: `import { CsvReader, CsvWriter, CsvReadResult, CSV_READ_INPUT_EMPTY, CSV_READ_OUTPUT_FULL, CSV_READ_FIELD, CSV_READ_RECORD_END, CSV_READ_END, countCsvRecords, countCsvFieldsInLine } from "csv-core";`
- **Source**: `crates.io:csv-core` v0.2 (MIT, zero-alloc streaming RFC 4180 DFA, `undefined` scratch buffers).

## Core Signatures

```cp
export class CsvReadResult {
    status: i32, bytes_read: usize, bytes_written: usize,
}

export class CsvReader {
    delimiter: u8, quote: u8, quoting: bool, state: u8, line_num: u64,
    fn new(): CsvReader;
    fn withDelimiter(delim: u8): CsvReader;
    fn line(): u64;
    fn reset(_: auto);
    fn readField(_: auto, input: &[]u8, out: []u8): CsvReadResult;
}

export class CsvWriter {
    delimiter: u8, quote: u8, always_quote: bool,
    fn new(): CsvWriter;
    fn withDelimiter(delim: u8): CsvWriter;
    fn writeField(field: &[]u8, out: []u8): usize;
    fn writeRecord(fields: &[]string, out: []u8): usize;
}

export fn countCsvRecords(input: &[]u8): usize;
export fn countCsvFieldsInLine(input: &[]u8): usize;
```

## Status Constants (`i32`)

`CSV_READ_INPUT_EMPTY = 0`, `CSV_READ_OUTPUT_FULL = 1`, `CSV_READ_FIELD = 2`, `CSV_READ_RECORD_END = 3`, `CSV_READ_END = 4`.

## Minimal Pattern

```cp
let mut r = CsvReader.new();
let mut out: [256]u8 = undefined;
let mut pos: usize = 0;
while (true) {
    let res = r.readField(buf[pos..buf.len], out[0..256]);
    pos += res.bytes_read;
    if (res.status == CSV_READ_END) { break; }
    if (res.status == CSV_READ_FIELD) { /* use out[0..res.bytes_written] */ }
}
```

## Pitfalls

1. **Always advance by `res.bytes_read`**: Even on `CSV_READ_OUTPUT_FULL`, partial progress was consumed; advance `pos += res.bytes_read` and flush `out[0..res.bytes_written]` before retrying.
2. **EOF flush**: Pass an empty slice (`buf[len..len]`) once all input is consumed so `CsvReader` emits the final `CSV_READ_FIELD` / `CSV_READ_RECORD_END` / `CSV_READ_END`.
3. **Mutable `CsvReader`**: Must be `let mut r = CsvReader.new()` because `readField` and `reset` mutate DFA state.
4. **Custom delimiter**: Use `CsvReader.withDelimiter('\t' as u8)` or `CsvWriter.withDelimiter(';' as u8)`.
