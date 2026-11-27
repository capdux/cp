// @cap/wordwrap — Zero-heap column-width text word wrapper

fn wwrpIsInlineWs(c: u8): bool {
    return c == 32 || c == 9;
}

fn wwrpPushByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn wwrpPushSpaces(out: []u8, pos: usize, count: usize): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < count && p < out.len) : (i += 1) {
        out[p] = 32;
        p += 1;
    }
    return p;
}

fn wwrpPushSlice(out: []u8, pos: usize, s: string): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

struct WwrpEmitRes {
    pos: usize,
    line_len: usize,
}

fn wwrpEmitWord(
    word: string,
    start_col: usize,
    max_w: usize,
    hard_mode: bool,
    out: []u8,
    start_pos: usize,
    cur_line_len: usize,
): WwrpEmitRes {
    let pos: usize = start_pos;
    let line_len: usize = cur_line_len;
    const wlen: usize = word.len;

    if (line_len > 0) {
        if (line_len + 1 + wlen <= max_w) {
            pos = wwrpPushByte(out, pos, 32);
            pos = wwrpPushSlice(out, pos, word);
            return WwrpEmitRes{ pos: pos, line_len: line_len + 1 + wlen };
        }
        // Start a new line
        pos = wwrpPushByte(out, pos, 10);
        line_len = 0;
    }

    // Beginning of line (line_len == 0)
    if (!hard_mode || wlen <= max_w) {
        pos = wwrpPushSpaces(out, pos, start_col);
        pos = wwrpPushSlice(out, pos, word);
        return WwrpEmitRes{ pos: pos, line_len: wlen };
    }

    // Hard mode: break overlong word into chunks of max_w
    let offset: usize = 0;
    while (offset < wlen) {
        if (offset > 0) {
            pos = wwrpPushByte(out, pos, 10);
        }
        pos = wwrpPushSpaces(out, pos, start_col);
        const rem: usize = wlen - offset;
        const take: usize = if (rem < max_w) rem else max_w;
        pos = wwrpPushSlice(out, pos, word[offset..offset + take]);
        line_len = take;
        offset += take;
    }

    return WwrpEmitRes{ pos: pos, line_len: line_len };
}

pub fn wrapText(input: string, start_col: usize, stop_col: usize, hard_mode: bool, out: []u8): usize {
    const n: usize = input.len;
    if (n == 0) {
        return 0;
    }
    const max_w: usize = if (stop_col > start_col) stop_col - start_col else 1;
    let pos: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;

    while (i <= n) : (i += 1) {
        if (i == n || input[i] == 10) {
            let line_end: usize = i;
            if (line_end > line_start && input[line_end - 1] == 13) {
                line_end -= 1;
            }

            if (line_start > 0) {
                pos = wwrpPushByte(out, pos, 10);
            }

            const line: string = input[line_start..line_end];
            let line_len: usize = 0;
            let k: usize = 0;
            while (k < line.len) {
                while (k < line.len && wwrpIsInlineWs(line[k])) : (k += 1) {}
                if (k >= line.len) {
                    break;
                }
                const wstart: usize = k;
                while (k < line.len && !wwrpIsInlineWs(line[k])) : (k += 1) {}
                const res = wwrpEmitWord(line[wstart..k], start_col, max_w, hard_mode, out, pos, line_len);
                pos = res.pos;
                line_len = res.line_len;
            }

            line_start = i + 1;
        }
    }

    return pos;
}
