// Type definitions for CP

export class CsvReadResult {
    status: i32,
    bytes_read: usize,
    bytes_written: usize,
}

export fn csvcNeedsQuote(field: &[]u8, delim: u8, quote: u8, always_quote: bool): bool;
export class CsvReader {
    delimiter: u8,
    quote: u8,
    quoting: bool,
    state: u8,
    line_num: u64,
    fn new(): CsvReader;
    fn withDelimiter(delim: u8): CsvReader;
    fn line(): u64;
    fn reset(_: auto);
    fn readField(_: auto, input: &[]u8, out: []u8): CsvReadResult;
}

export class CsvWriter {
    delimiter: u8,
    quote: u8,
    always_quote: bool,
    fn new(): CsvWriter;
    fn withDelimiter(delim: u8): CsvWriter;
    fn writeField(field: &[]u8, out: []u8): usize;
    fn writeRecord(fields: &[]string, out: []u8): usize;
}

export fn countCsvRecords(input: &[]u8): usize;
export fn countCsvFieldsInLine(input: &[]u8): usize;
