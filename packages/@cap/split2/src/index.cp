// @cap/split2 — Zero-allocation streaming line splitter across chunk boundaries
// Inspired by npm:split2 (https://github.com/mcollina/split2)

pub const SPLIT2_BUF_CAP: usize = 4096;

fn sp2Min(a: usize, b: usize): usize {
    return if (a < b) a else b;
}

fn sp2CopyBytes(dst: []u8, dst_off: usize, src: []const u8, n: usize) {
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[dst_off + i] = src[i];
    }
}

/// Zero-heap streaming line splitter with a fixed 4096-byte partial-line buffer.
/// Handles both LF (`\n`) and CRLF (`\r\n`), including split across chunk boundaries.
pub struct LineSplitter {
    buf: [4096]u8,
    len: usize,
    max_line_len: usize,
    skip_empty: bool,
    is_overflowed: bool,

    static fn new(): LineSplitter {
        return LineSplitter{
            buf: [0; 4096],
            len: 0,
            max_line_len: SPLIT2_BUF_CAP,
            skip_empty: false,
            is_overflowed: false,
        };
    }

    static fn withMaxLen(max_len: usize): LineSplitter {
        const clamped: usize = sp2Min(max_len, SPLIT2_BUF_CAP);
        return LineSplitter{
            buf: [0; 4096],
            len: 0,
            max_line_len: clamped,
            skip_empty: false,
            is_overflowed: false,
        };
    }

    /// Feeds a chunk of bytes into the splitter.
    /// Emits complete lines into `out_lines_buf`, recording each line's start offset in
    /// `out_starts` and byte length in `out_lens`. Returns the number of complete lines emitted.
    fn pushChunk(
        *_,
        chunk: []const u8,
        out_lines_buf: []u8,
        out_starts: []usize,
        out_lens: []usize
    ): usize {
        let emitted: usize = 0;
        let out_pos: usize = 0;
        const max_out: usize = sp2Min(out_starts.len, out_lens.len);
        const cap_limit: usize = sp2Min(_.max_line_len, SPLIT2_BUF_CAP);

        let i: usize = 0;
        while (i < chunk.len) : (i += 1) {
            const b: u8 = chunk[i];
            if (b == 0x0A) {
                let line_len: usize = _.len;
                if (line_len > 0 && _.buf[line_len - 1] == 0x0D) {
                    line_len -= 1;
                }
                if (!(_.skip_empty && line_len == 0)) {
                    if (emitted < max_out && out_pos + line_len <= out_lines_buf.len) {
                        sp2CopyBytes(out_lines_buf, out_pos, _.buf[0..line_len], line_len);
                        out_starts[emitted] = out_pos;
                        out_lens[emitted] = line_len;
                        out_pos += line_len;
                        emitted += 1;
                    }
                }
                _.len = 0;
            } else {
                if (_.len < cap_limit) {
                    _.buf[_.len] = b;
                    _.len += 1;
                } else {
                    _.is_overflowed = true;
                }
            }
        }

        return emitted;
    }

    /// Flushes any remaining partial line at end-of-stream into `out_line`.
    /// Returns the number of bytes written (0 if nothing pending).
    fn flush(*_, out_line: []u8): usize {
        if (_.len == 0) {
            return 0;
        }
        const n: usize = sp2Min(_.len, out_line.len);
        sp2CopyBytes(out_line, 0, _.buf[0..n], n);
        _.len = 0;
        return n;
    }

    fn pendingLen(): usize {
        return _.len;
    }

    fn overflowed(): bool {
        return _.is_overflowed;
    }

    fn reset(*_) {
        _.len = 0;
        _.is_overflowed = false;
    }
}

/// Stateless zero-copy line splitter supporting `\n` and `\r\n`.
/// Writes line slices into `out_slices` and returns the number of lines produced.
pub fn splitLines(input: string, out_slices: []string): usize {
    if (input.len == 0) {
        return 0;
    }
    let count: usize = 0;
    let start: usize = 0;
    let i: usize = 0;

    while (i < input.len) : (i += 1) {
        if (input[i] == 0x0A) {
            let end: usize = i;
            if (end > start && input[end - 1] == 0x0D) {
                end -= 1;
            }
            if (count < out_slices.len) {
                out_slices[count] = input[start..end];
                count += 1;
            }
            start = i + 1;
        }
    }

    if (start < input.len && count < out_slices.len) {
        out_slices[count] = input[start..input.len];
        count += 1;
    }

    return count;
}
