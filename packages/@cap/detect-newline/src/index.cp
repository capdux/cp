// @cap/detect-newline — Zero-allocation newline detector & LF/CRLF normalizer
// Inspired by Sindre Sorhus `detect-newline`
// MIT License

pub const EOL_NONE: u8 = 0;
pub const EOL_LF: u8 = 1;
pub const EOL_CRLF: u8 = 2;
pub const EOL_CR: u8 = 3;

pub struct NewlineStats {
    pub lf_count: usize,
    pub crlf_count: usize,
    pub cr_count: usize,
    pub dominant: u8,
}

pub fn analyzeNewlines(input: string): NewlineStats {
    let lf_count: usize = 0;
    let crlf_count: usize = 0;
    let cr_count: usize = 0;

    let i: usize = 0;
    while (i < input.len) {
        let ch: u8 = input[i];
        if (ch == 13) {
            if (i + 1 < input.len && input[i + 1] == 10) {
                crlf_count = crlf_count + 1;
                i = i + 2;
            } else {
                cr_count = cr_count + 1;
                i = i + 1;
            }
        } else if (ch == 10) {
            lf_count = lf_count + 1;
            i = i + 1;
        } else {
            i = i + 1;
        }
    }

    let dom: u8 = EOL_NONE;
    if (lf_count > 0 || crlf_count > 0 || cr_count > 0) {
        if (crlf_count > lf_count && crlf_count >= cr_count) {
            dom = EOL_CRLF;
        } else if (lf_count >= crlf_count && lf_count >= cr_count) {
            dom = EOL_LF;
        } else {
            dom = EOL_CR;
        }
    }

    return NewlineStats {
        lf_count: lf_count,
        crlf_count: crlf_count,
        cr_count: cr_count,
        dominant: dom,
    };
}

pub fn detectNewline(input: string): string {
    let st: NewlineStats = analyzeNewlines(input);
    if (st.crlf_count == 0 && st.lf_count == 0) {
        return "";
    }
    if (st.crlf_count > st.lf_count) {
        return "\r\n";
    }
    return "\n";
}

pub fn detectNewlineGraceful(input: string): string {
    let st: NewlineStats = analyzeNewlines(input);
    if (st.crlf_count > st.lf_count) {
        return "\r\n";
    }
    return "\n";
}

pub fn normalizeToLf(input: string, out: []u8): usize {
    if (input.len == 0) {
        return 0;
    }
    let need: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == 13 && i + 1 < input.len && input[i + 1] == 10) {
            need = need + 1;
            i = i + 2;
        } else {
            need = need + 1;
            i = i + 1;
        }
    }
    if (out.len < need) {
        return 0;
    }

    let pos: usize = 0;
    let k: usize = 0;
    while (k < input.len) {
        let ch: u8 = input[k];
        if (ch == 13) {
            out[pos] = 10;
            pos = pos + 1;
            if (k + 1 < input.len && input[k + 1] == 10) {
                k = k + 2;
            } else {
                k = k + 1;
            }
        } else {
            out[pos] = ch;
            pos = pos + 1;
            k = k + 1;
        }
    }
    return pos;
}

pub fn normalizeToCrlf(input: string, out: []u8): usize {
    if (input.len == 0) {
        return 0;
    }
    let need: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        let ch: u8 = input[i];
        if (ch == 13) {
            need = need + 2;
            if (i + 1 < input.len && input[i + 1] == 10) {
                i = i + 2;
            } else {
                i = i + 1;
            }
        } else if (ch == 10) {
            need = need + 2;
            i = i + 1;
        } else {
            need = need + 1;
            i = i + 1;
        }
    }
    if (out.len < need) {
        return 0;
    }

    let pos: usize = 0;
    let k: usize = 0;
    while (k < input.len) {
        let ch: u8 = input[k];
        if (ch == 13) {
            out[pos] = 13;
            out[pos + 1] = 10;
            pos = pos + 2;
            if (k + 1 < input.len && input[k + 1] == 10) {
                k = k + 2;
            } else {
                k = k + 1;
            }
        } else if (ch == 10) {
            out[pos] = 13;
            out[pos + 1] = 10;
            pos = pos + 2;
            k = k + 1;
        } else {
            out[pos] = ch;
            pos = pos + 1;
            k = k + 1;
        }
    }
    return pos;
}
