// @cap/anstyle-parse — Zero-heap VT100/ANSI escape sequence state-machine parser in pure CP
// MIT License — Clean-room implementation supporting Text, CSI, OSC, and ESC sequences

pub const TOKEN_TEXT: u8 = 0;
pub const TOKEN_CSI: u8 = 1;
pub const TOKEN_OSC: u8 = 2;
pub const TOKEN_ESC: u8 = 3;

const BYTE_ESC: u8 = 27; // 0x1B
const BYTE_BEL: u8 = 7;  // 0x07

pub struct AnsiToken {
    pub kind: u8,
    pub start: usize,
    pub len: usize,
    pub final_byte: u8,
}

pub struct SgrParams {
    pub params: [16]u16,
    pub count: usize,
    pub valid: bool,
}

/// Parse the next single ANSI token (Text, CSI, OSC, or ESC) starting at `pos`.
fn nextToken(input: string, pos: usize): AnsiToken {
    if (input[pos] != BYTE_ESC) {
        let j: usize = pos + 1;
        while (j < input.len && input[j] != BYTE_ESC) {
            j += 1;
        }
        return AnsiToken {
            kind: TOKEN_TEXT,
            start: pos,
            len: j - pos,
            final_byte: 0,
        };
    }

    // Bare trailing ESC at end of input
    if (pos + 1 >= input.len) {
        return AnsiToken {
            kind: TOKEN_ESC,
            start: pos,
            len: 1,
            final_byte: 0,
        };
    }

    const intro: u8 = input[pos + 1];

    // CSI: ESC [ <params 0x30..0x3F>* <intermediates 0x20..0x2F>* <final 0x40..0x7E>
    if (intro == 91) { // '['
        let j: usize = pos + 2;
        while (j < input.len && input[j] >= 0x30 && input[j] <= 0x3F) {
            j += 1;
        }
        while (j < input.len && input[j] >= 0x20 && input[j] <= 0x2F) {
            j += 1;
        }
        let fb: u8 = 0;
        if (j < input.len && input[j] >= 0x40 && input[j] <= 0x7E) {
            fb = input[j];
            j += 1;
        }
        return AnsiToken {
            kind: TOKEN_CSI,
            start: pos,
            len: j - pos,
            final_byte: fb,
        };
    }

    // OSC: ESC ] <payload> (BEL | ESC \)
    if (intro == 93) { // ']'
        let j: usize = pos + 2;
        let fb: u8 = 0;
        while (j < input.len) {
            if (input[j] == BYTE_BEL) {
                fb = BYTE_BEL;
                j += 1;
                break;
            }
            if (input[j] == BYTE_ESC && j + 1 < input.len && input[j + 1] == 92) { // ESC '\' (ST)
                fb = 92;
                j += 2;
                break;
            }
            j += 1;
        }
        return AnsiToken {
            kind: TOKEN_OSC,
            start: pos,
            len: j - pos,
            final_byte: fb,
        };
    }

    // Generic 2-byte / intermediate ESC sequence: ESC <intermediates 0x20..0x2F>* <final>
    let j: usize = pos + 1;
    while (j < input.len && input[j] >= 0x20 && input[j] <= 0x2F) {
        j += 1;
    }
    let fb: u8 = 0;
    if (j < input.len) {
        fb = input[j];
        j += 1;
    }
    return AnsiToken {
        kind: TOKEN_ESC,
        start: pos,
        len: j - pos,
        final_byte: fb,
    };
}

/// Tokenize an ANSI-encoded string into `out` without heap allocation.
/// Returns the number of tokens written (stops if `out` is full).
pub fn tokenizeAnsi(input: string, out: []AnsiToken): usize {
    let pos: usize = 0;
    let count: usize = 0;
    while (pos < input.len && count < out.len) {
        const tok: AnsiToken = nextToken(input, pos);
        out[count] = tok;
        count += 1;
        pos += tok.len;
    }
    return count;
}

/// Parse semicolon-separated numeric parameters from a CSI/SGR sequence (e.g. `ESC[1;38;5;196m`).
/// Empty parameter list (e.g. `ESC[m`) normalizes to a single parameter `[0]`.
pub fn parseCsiParams(csi_seq: string): SgrParams {
    let res = SgrParams {
        params: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        count: 0,
        valid: false,
    };
    if (csi_seq.len < 3) {
        return res;
    }
    if (csi_seq[0] != BYTE_ESC || csi_seq[1] != 91) {
        return res;
    }
    const final_b: u8 = csi_seq[csi_seq.len - 1];
    if (final_b < 0x40 || final_b > 0x7E) {
        return res;
    }

    const end_idx: usize = csi_seq.len - 1;
    if (end_idx == 2) {
        // ESC[m -> default parameter 0
        res.params[0] = 0;
        res.count = 1;
        res.valid = true;
        return res;
    }

    let i: usize = 2;
    let cur_val: u32 = 0;
    let count: usize = 0;

    while (i < end_idx) {
        const ch: u8 = csi_seq[i];
        if (ch >= 48 && ch <= 57) {
            cur_val = cur_val * 10 + ((ch - 48) as u32);
            if (cur_val > 65535) {
                cur_val = 65535;
            }
        } else if (ch == 59 || ch == 58) { // ';' or ':'
            if (count < 16) {
                res.params[count] = cur_val as u16;
                count += 1;
            }
            cur_val = 0;
        } else {
            return res;
        }
        i += 1;
    }

    if (count < 16) {
        res.params[count] = cur_val as u16;
        count += 1;
    }
    res.count = count;
    res.valid = true;
    return res;
}

/// Strip all ANSI CSI/OSC/ESC sequences from `input` and write plain text bytes to `out`.
/// Returns the number of visible bytes written to `out`.
pub fn stripAnsiToBuf(input: string, out: []u8): usize {
    let pos: usize = 0;
    let w: usize = 0;
    while (pos < input.len) {
        const tok: AnsiToken = nextToken(input, pos);
        if (tok.kind == TOKEN_TEXT) {
            let k: usize = 0;
            while (k < tok.len && w < out.len) {
                out[w] = input[tok.start + k];
                w += 1;
                k += 1;
            }
        }
        pos += tok.len;
    }
    return w;
}

/// Return true if `input` contains any ANSI escape introducer (`ESC`, 0x1B).
pub fn hasAnsi(input: string): bool {
    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == BYTE_ESC) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Return the total number of visible (non-ANSI) bytes in `input` without allocating.
pub fn visibleByteLen(input: string): usize {
    let pos: usize = 0;
    let total: usize = 0;
    while (pos < input.len) {
        const tok: AnsiToken = nextToken(input, pos);
        if (tok.kind == TOKEN_TEXT) {
            total += tok.len;
        }
        pos += tok.len;
    }
    return total;
}
