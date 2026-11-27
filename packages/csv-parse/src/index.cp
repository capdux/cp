// @cap/csv-line — Zero-heap RFC 4180 CSV/TSV record parser & serializer
// MIT License (clean-room CP implementation)

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
            // Quoted RFC 4180 field
            pos += 1;
            let closed: bool = false;
            while (pos < src.len) {
                const c: u8 = src[pos];
                if (c == '"' as u8) {
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
                if (out_pos >= out_buf.len) {
                    return fail;
                }
                out_buf[out_pos] = c;
                out_pos += 1;
                pos += 1;
            }
            if (!closed) {
                return fail;
            }
            if (pos < src.len && src[pos] != delim && src[pos] != '\r' as u8 && src[pos] != '\n' as u8) {
                return fail;
            }
        } else {
            // Unquoted field
            while (pos < src.len) {
                const c: u8 = src[pos];
                if (c == delim || c == '\r' as u8 || c == '\n' as u8) {
                    break;
                }
                if (c == '"' as u8) {
                    return fail;
                }
                if (out_pos >= out_buf.len) {
                    return fail;
                }
                out_buf[out_pos] = c;
                out_pos += 1;
                pos += 1;
            }
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
    let needs_quote: bool = false;
    let i: usize = 0;
    while (i < field.len) {
        const c: u8 = field[i];
        if (c == delim || c == '"' as u8 || c == '\n' as u8 || c == '\r' as u8) {
            needs_quote = true;
            break;
        }
        i += 1;
    }

    if (!needs_quote) {
        if (field.len > dst.len) {
            return -1;
        }
        i = 0;
        while (i < field.len) {
            dst[i] = field[i];
            i += 1;
        }
        return field.len as i64;
    }

    let pos: usize = 0;
    if (pos >= dst.len) {
        return -1;
    }
    dst[pos] = '"' as u8;
    pos += 1;

    i = 0;
    while (i < field.len) {
        const c: u8 = field[i];
        if (c == '"' as u8) {
            if (2 > dst.len - pos) {
                return -1;
            }
            dst[pos] = '"' as u8;
            dst[pos + 1] = '"' as u8;
            pos += 2;
        } else {
            if (pos >= dst.len) {
                return -1;
            }
            dst[pos] = c;
            pos += 1;
        }
        i += 1;
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
            let i: usize = 0;
            while (i < field.len) {
                tmp[i + 1] = field[i];
                i += 1;
            }
            return formatField(tmp[0..(field.len + 1)], delim, dst);
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
