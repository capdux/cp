// Type definitions for CP

export class CsvFieldSpan {
    start: usize,
    len: usize,
}

export class CsvParseResult {
    field_count: usize,
    next_pos: usize,
    ok: bool,
}

export fn parseRecord(src: &[]u8, start_pos: usize, delim: u8, out_buf: []u8, out_spans: []CsvFieldSpan): CsvParseResult;
export fn formatField(field: &[]u8, delim: u8, dst: []u8): i64;
export fn formatFieldSafe(field: &[]u8, delim: u8, dst: []u8): i64;
export fn formatRecord(fields: &[]&[]u8, delim: u8, dst: []u8): i64;
