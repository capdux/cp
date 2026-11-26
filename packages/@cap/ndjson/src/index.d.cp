// Type definitions for CP

export class NdjsonLine {
    start: usize,
    len: usize,
    found: bool,
}

export fn isBlankSlice(s: &[]u8): bool;
export fn nextLine(src: &[]u8, start_pos: usize, out_next_pos: &mut usize): NdjsonLine;
export fn countLines(src: &[]u8): usize;
export class NdjsonSplitter {
    buf: [1024]u8,
    pos: usize,
    ready_len: usize,
    overflowed: bool,
    fn init(): NdjsonSplitter;
    fn reset(_: auto);
    fn pushByte(_: auto, b: u8): i64;
    fn currentLine(): &[]u8;
    fn flush(_: auto): i64;
}

export fn writeLine(json_record: &[]u8, dst: []u8): i64;
