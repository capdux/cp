// Type definitions for CP

export fn sp2Min(a: usize, b: usize): usize;
export fn sp2CopyBytes(dst: []u8, dst_off: usize, src: &[]u8, n: usize);
export class LineSplitter {
    buf: [4096]u8,
    len: usize,
    max_line_len: usize,
    skip_empty: bool,
    is_overflowed: bool,
    fn new(): LineSplitter;
    fn withMaxLen(max_len: usize): LineSplitter;
    fn pushChunk(_: auto, chunk: &[]u8, out_lines_buf: []u8, out_starts: []usize, out_lens: []usize): usize;
    fn flush(_: auto, out_line: []u8): usize;
    fn pendingLen(): usize;
    fn overflowed(): bool;
    fn reset(_: auto);
}

export fn splitLines(input: string, out_slices: []string): usize;
