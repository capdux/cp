// @cap/pem-parse — Zero-heap RFC 7468 PEM parser, iterator & 64-column encoder
// MIT License (clean-room CP implementation)

pub struct PemBlock {
    pub label: string,
    pub body_b64: string,
    pub next_pos: usize,
    pub valid: bool,
}

fn bytesEq(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn startsWith(s: string, prefix: string) -> bool {
    if (s.len < prefix.len) {
        return false;
    }
    return bytesEq(s[0..prefix.len], prefix);
}

fn endsWith(s: string, suffix: string) -> bool {
    if (s.len < suffix.len) {
        return false;
    }
    return bytesEq(s[(s.len - suffix.len)..s.len], suffix);
}

pub fn isValidLabel(label: string) -> bool {
    if (label.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < label.len) {
        const c: u8 = label[i];
        const ok: bool = (c >= 'A' as u8 && c <= 'Z' as u8)
            || (c >= '0' as u8 && c <= '9' as u8)
            || c == ' ' as u8
            || c == '-' as u8;
        if (!ok) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn nextBlock(input: string, start: usize) -> PemBlock {
    const fail = PemBlock {
        label: "",
        body_b64: "",
        next_pos: start,
        valid: false,
    };

    const begin_pfx = "-----BEGIN ";
    const end_pfx = "-----END ";
    const dashes = "-----";

    let pos: usize = start;
    let found_begin: bool = false;
    let label: string = "";
    let body_start: usize = 0;

    while (pos < input.len) {
        let line_end: usize = pos;
        while (line_end < input.len && input[line_end] != '\n' as u8) {
            line_end += 1;
        }
        let raw_end: usize = line_end;
        if (raw_end > pos && input[raw_end - 1] == '\r' as u8) {
            raw_end -= 1;
        }
        let next_line: usize = line_end;
        if (next_line < input.len && input[next_line] == '\n' as u8) {
            next_line += 1;
        }

        const line: string = input[pos..raw_end];
        if (!found_begin) {
            if (line.len > begin_pfx.len + dashes.len
                && startsWith(line, begin_pfx)
                && endsWith(line, dashes)) {
                const candidate: string = line[begin_pfx.len..(line.len - dashes.len)];
                if (isValidLabel(candidate)) {
                    found_begin = true;
                    label = candidate;
                    body_start = next_line;
                }
            }
        } else {
            if (line.len > end_pfx.len + dashes.len
                && startsWith(line, end_pfx)
                && endsWith(line, dashes)) {
                const end_label: string = line[end_pfx.len..(line.len - dashes.len)];
                if (!bytesEq(label, end_label)) {
                    return fail;
                }
                return PemBlock {
                    label: label,
                    body_b64: input[body_start..pos],
                    next_pos: next_line,
                    valid: true,
                };
            }
        }

        pos = next_line;
    }

    return fail;
}

pub fn countBlocks(input: string) -> usize {
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < input.len) {
        const blk: PemBlock = nextBlock(input, pos);
        if (!blk.valid) {
            break;
        }
        count += 1;
        if (blk.next_pos <= pos) {
            break;
        }
        pos = blk.next_pos;
    }
    return count;
}

fn isB64Ws(c: u8) -> bool {
    return c == ' ' as u8 || c == '\t' as u8 || c == '\r' as u8 || c == '\n' as u8;
}

fn b64DecVal(c: u8) -> i32 {
    if (c >= 'A' as u8 && c <= 'Z' as u8) {
        return (c - ('A' as u8)) as i32;
    }
    if (c >= 'a' as u8 && c <= 'z' as u8) {
        return (c - ('a' as u8) + 26) as i32;
    }
    if (c >= '0' as u8 && c <= '9' as u8) {
        return (c - ('0' as u8) + 52) as i32;
    }
    if (c == '+' as u8) {
        return 62;
    }
    if (c == '/' as u8) {
        return 63;
    }
    return -1;
}

fn b64EncChar(v: u8) -> u8 {
    const alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    return alphabet[v as usize];
}

pub fn decodeBase64Body(body_b64: string, out_der: []u8) -> i64 {
    let quad: [4]u8 = undefined;
    let q_len: usize = 0;
    let out_pos: usize = 0;
    let saw_pad: bool = false;

    let i: usize = 0;
    while (i < body_b64.len) {
        const c: u8 = body_b64[i];
        i += 1;
        if (isB64Ws(c)) {
            continue;
        }
        if (saw_pad) {
            return -1;
        }
        quad[q_len] = c;
        q_len += 1;
        if (q_len == 4) {
            const v0: i32 = b64DecVal(quad[0]);
            const v1: i32 = b64DecVal(quad[1]);
            if (v0 < 0 || v1 < 0) {
                return -1;
            }
            if (quad[2] == '=' as u8) {
                if (quad[3] != '=' as u8 || out_pos >= out_der.len) {
                    return -1;
                }
                const t: u32 = ((v0 as u32) << 18) | ((v1 as u32) << 12);
                out_der[out_pos] = ((t >> 16) & 0xFF) as u8;
                out_pos += 1;
                saw_pad = true;
            } else {
                const v2: i32 = b64DecVal(quad[2]);
                if (v2 < 0) {
                    return -1;
                }
                if (quad[3] == '=' as u8) {
                    if (out_pos + 2 > out_der.len) {
                        return -1;
                    }
                    const t: u32 = ((v0 as u32) << 18) | ((v1 as u32) << 12) | ((v2 as u32) << 6);
                    out_der[out_pos] = ((t >> 16) & 0xFF) as u8;
                    out_der[out_pos + 1] = ((t >> 8) & 0xFF) as u8;
                    out_pos += 2;
                    saw_pad = true;
                } else {
                    const v3: i32 = b64DecVal(quad[3]);
                    if (v3 < 0 || out_pos + 3 > out_der.len) {
                        return -1;
                    }
                    const t: u32 = ((v0 as u32) << 18)
                        | ((v1 as u32) << 12)
                        | ((v2 as u32) << 6)
                        | (v3 as u32);
                    out_der[out_pos] = ((t >> 16) & 0xFF) as u8;
                    out_der[out_pos + 1] = ((t >> 8) & 0xFF) as u8;
                    out_der[out_pos + 2] = (t & 0xFF) as u8;
                    out_pos += 3;
                }
            }
            q_len = 0;
        }
    }

    if (q_len != 0) {
        return -1;
    }
    return out_pos as i64;
}

fn appendSlice(out: []u8, pos: usize, s: string) -> i64 {
    if (s.len > out.len - pos) {
        return -1;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos + i] = s[i];
        i += 1;
    }
    return (pos + s.len) as i64;
}

fn appendB64Wrapped(out: []u8, pos: usize, col: *usize, c: u8) -> i64 {
    let cur: usize = pos;
    if (cur >= out.len) {
        return -1;
    }
    out[cur] = c;
    cur += 1;
    col.* += 1;
    if (col.* == 64) {
        if (cur >= out.len) {
            return -1;
        }
        out[cur] = '\n' as u8;
        cur += 1;
        col.* = 0;
    }
    return cur as i64;
}

pub fn encodePem(label: string, der: []const u8, out: []u8) -> usize {
    if (!isValidLabel(label)) {
        return 0;
    }

    let p: i64 = appendSlice(out, 0, "-----BEGIN ");
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, label);
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, "-----\n");
    if (p < 0) {
        return 0;
    }

    let col: usize = 0;
    let i: usize = 0;
    while (i + 3 <= der.len) {
        const v: u32 = ((der[i] as u32) << 16)
            | ((der[i + 1] as u32) << 8)
            | (der[i + 2] as u32);
        p = appendB64Wrapped(out, p as usize, &col, b64EncChar(((v >> 18) & 0x3F) as u8));
        if (p < 0) {
            return 0;
        }
        p = appendB64Wrapped(out, p as usize, &col, b64EncChar(((v >> 12) & 0x3F) as u8));
        if (p < 0) {
            return 0;
        }
        p = appendB64Wrapped(out, p as usize, &col, b64EncChar(((v >> 6) & 0x3F) as u8));
        if (p < 0) {
            return 0;
        }
        p = appendB64Wrapped(out, p as usize, &col, b64EncChar((v & 0x3F) as u8));
        if (p < 0) {
            return 0;
        }
        i += 3;
    }

    const rem: usize = der.len - i;
    if (rem == 1) {
        const v: u32 = (der[i] as u32) << 16;
        p = appendB64Wrapped(out, p as usize, &col, b64EncChar(((v >> 18) & 0x3F) as u8));
        if (p < 0) {
            return 0;
        }
        p = appendB64Wrapped(out, p as usize, &col, b64EncChar(((v >> 12) & 0x3F) as u8));
        if (p < 0) {
            return 0;
        }
        p = appendB64Wrapped(out, p as usize, &col, '=' as u8);
        if (p < 0) {
            return 0;
        }
        p = appendB64Wrapped(out, p as usize, &col, '=' as u8);
        if (p < 0) {
            return 0;
        }
    } else if (rem == 2) {
        const v: u32 = ((der[i] as u32) << 16) | ((der[i + 1] as u32) << 8);
        p = appendB64Wrapped(out, p as usize, &col, b64EncChar(((v >> 18) & 0x3F) as u8));
        if (p < 0) {
            return 0;
        }
        p = appendB64Wrapped(out, p as usize, &col, b64EncChar(((v >> 12) & 0x3F) as u8));
        if (p < 0) {
            return 0;
        }
        p = appendB64Wrapped(out, p as usize, &col, b64EncChar(((v >> 6) & 0x3F) as u8));
        if (p < 0) {
            return 0;
        }
        p = appendB64Wrapped(out, p as usize, &col, '=' as u8);
        if (p < 0) {
            return 0;
        }
    }

    if (col > 0) {
        p = appendSlice(out, p as usize, "\n");
        if (p < 0) {
            return 0;
        }
    }

    p = appendSlice(out, p as usize, "-----END ");
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, label);
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, "-----\n");
    if (p < 0) {
        return 0;
    }
    return p as usize;
}
