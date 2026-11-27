// @cap/csv-line — Zero-heap RFC 4180 CSV/TSV record parser & serializer
// MIT License (clean-room CP implementation)
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning and @memcpy bulk copy.

type CsvpVec16 = @Vector(16, u8);

fn csvpFindQuote(src: []const u8, start: usize) -> usize {
    let i: usize = start;
    const len = src.len;
    const v_q: CsvpVec16 = @splat('"' as u8);
    while (i + 16 <= len) {
        const v: CsvpVec16 = src[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_q)) {
            let k: usize = 0;
            while (k < 16) {
                if (src[i + k] == '"' as u8) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (src[i] == '"' as u8) {
            return i;
        }
        i += 1;
    }
    return len;
}

fn csvpFindUnquotedStop(src: []const u8, start: usize, delim: u8) -> usize {
    let i: usize = start;
    const len = src.len;
    const v_del: CsvpVec16 = @splat(delim);
    const v_cr: CsvpVec16 = @splat('\r' as u8);
    const v_lf: CsvpVec16 = @splat('\n' as u8);
    const v_q: CsvpVec16 = @splat('"' as u8);
    while (i + 16 <= len) {
        const v: CsvpVec16 = src[i .. i + 16][0..16].*;
        if (
            @reduce(.Or, v == v_del) ||
            @reduce(.Or, v == v_cr) ||
            @reduce(.Or, v == v_lf) ||
            @reduce(.Or, v == v_q)
        ) {
            let k: usize = 0;
            while (k < 16) {
                const c: u8 = src[i + k];
                if (c == delim || c == '\r' as u8 || c == '\n' as u8 || c == '"' as u8) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const c: u8 = src[i];
        if (c == delim || c == '\r' as u8 || c == '\n' as u8 || c == '"' as u8) {
            return i;
        }
        i += 1;
    }
    return len;
}

pub struct CsvFieldSpan {
    pub start: usize,
    pub len: usize,
}

pub struct CsvParseResult {
    pub field_count: usize,
    pub next_pos: usize,
    pub ok: bool,
}

pub fn parseRecord(
    src: []const u8,
    start_pos: usize,
    delim: u8,
    out_buf: []u8,
    out_spans: []CsvFieldSpan,
) -> CsvParseResult {
    const fail = CsvParseResult { field_count: 0, next_pos: start_pos, ok: false };
    if (start_pos >= src.len) {
        return CsvParseResult { field_count: 0, next_pos: start_pos, ok: true };
    }

    let pos: usize = start_pos;
    let out_pos: usize = 0;
    let count: usize = 0;

    while (true) {
        if (count >= out_spans.len) {
            return fail;
        }
        const f_start: usize = out_pos;

        if (pos < src.len && src[pos] == '"' as u8) {
            // Quoted RFC 4180 field — SIMD scan to next quote + @memcpy chunk
            pos += 1;
            let closed: bool = false;
            while (pos < src.len) {
                const q_pos: usize = csvpFindQuote(src, pos);
                const chunk: usize = q_pos - pos;
                if (chunk > 0) {
                    if (chunk > out_buf.len - out_pos) {
                        return fail;
                    }
                    @memcpy(out_buf[out_pos .. out_pos + chunk], src[pos .. q_pos]);
                    out_pos += chunk;
                    pos = q_pos;
                }
                if (pos >= src.len) {
                    break;
                }
                // src[pos] == '"'
                if (pos + 1 < src.len && src[pos + 1] == '"' as u8) {
                    if (out_pos >= out_buf.len) {
                        return fail;
                    }
                    out_buf[out_pos] = '"' as u8;
                    out_pos += 1;
                    pos += 2;
                    continue;
                }
                pos += 1;
                closed = true;
                break;
            }
            if (!closed) {
                return fail;
            }
            if (pos < src.len && src[pos] != delim && src[pos] != '\r' as u8 && src[pos] != '\n' as u8) {
                return fail;
            }
        } else {
            // Unquoted field — SIMD scan for delim / CR / LF / quote + @memcpy
            const stop: usize = csvpFindUnquotedStop(src, pos, delim);
            if (stop < src.len && src[stop] == '"' as u8) {
                return fail;
            }
            const chunk: usize = stop - pos;
            if (chunk > out_buf.len - out_pos) {
                return fail;
            }
            if (chunk > 0) {
                @memcpy(out_buf[out_pos .. out_pos + chunk], src[pos .. stop]);
                out_pos += chunk;
            }
            pos = stop;
        }

        out_spans[count] = CsvFieldSpan {
            start: f_start,
            len: out_pos - f_start,
        };
        count += 1;

        if (pos < src.len && src[pos] == delim) {
            pos += 1;
            continue;
        }

        if (pos < src.len && src[pos] == '\r' as u8) {
            pos += 1;
            if (pos < src.len && src[pos] == '\n' as u8) {
                pos += 1;
            }
        } else if (pos < src.len && src[pos] == '\n' as u8) {
            pos += 1;
        }

        return CsvParseResult {
            field_count: count,
            next_pos: pos,
            ok: true,
        };
    }

    return fail;
}

pub fn formatField(field: []const u8, delim: u8, dst: []u8) -> i64 {
    const first_special: usize = csvpFindUnquotedStop(field, 0, delim);
    if (first_special >= field.len) {
        if (field.len > dst.len) {
            return -1;
        }
        if (field.len > 0) {
            @memcpy(dst[0 .. field.len], field[0 .. field.len]);
        }
        return field.len as i64;
    }

    let pos: usize = 0;
    if (pos >= dst.len) {
        return -1;
    }
    dst[pos] = '"' as u8;
    pos += 1;

    let i: usize = 0;
    while (i < field.len) {
        const q_pos: usize = csvpFindQuote(field, i);
        const chunk: usize = q_pos - i;
        if (chunk > 0) {
            if (chunk > dst.len - pos) {
                return -1;
            }
            @memcpy(dst[pos .. pos + chunk], field[i .. q_pos]);
            pos += chunk;
            i = q_pos;
        }
        if (i < field.len) {
            if (2 > dst.len - pos) {
                return -1;
            }
            dst[pos] = '"' as u8;
            dst[pos + 1] = '"' as u8;
            pos += 2;
            i += 1;
        }
    }

    if (pos >= dst.len) {
        return -1;
    }
    dst[pos] = '"' as u8;
    pos += 1;
    return pos as i64;
}

pub fn formatFieldSafe(field: []const u8, delim: u8, dst: []u8) -> i64 {
    if (field.len > 0) {
        const c0: u8 = field[0];
        if (c0 == '=' as u8 || c0 == '+' as u8 || c0 == '-' as u8 || c0 == '@' as u8) {
            let tmp: [512]u8 = undefined;
            if (field.len + 1 > 512) {
                return -1;
            }
            tmp[0] = '\'' as u8;
            @memcpy(tmp[1 .. field.len + 1], field[0 .. field.len]);
            return formatField(tmp[0 .. (field.len + 1)], delim, dst);
        }
    }
    return formatField(field, delim, dst);
}

pub fn formatRecord(fields: []const []const u8, delim: u8, dst: []u8) -> i64 {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < fields.len) {
        if (i > 0) {
            if (pos >= dst.len) {
                return -1;
            }
            dst[pos] = delim;
            pos += 1;
        }
        const w: i64 = formatField(fields[i], delim, dst[pos..dst.len]);
        if (w < 0) {
            return -1;
        }
        pos += w as usize;
        i += 1;
    }
    return pos as i64;
}
