// @cap/content-disposition — Zero-heap RFC 6266 / RFC 5987 HTTP Content-Disposition codec
// MIT License (clean-room CP implementation)

pub const DISP_INVALID: u8 = 0;
pub const DISP_INLINE: u8 = 1;
pub const DISP_ATTACHMENT: u8 = 2;
pub const DISP_FORM_DATA: u8 = 3;

pub struct ParsedDisposition {
    pub kind: u8,
    pub filename_len: usize,
    pub is_utf8: bool,
    pub ok: bool,
}

fn isWs(c: u8) -> bool {
    return c == ' ' as u8 || c == '\t' as u8;
}

fn toLowerAscii(c: u8) -> u8 {
    if (c >= 'A' as u8 && c <= 'Z' as u8) {
        return c + 32;
    }
    return c;
}

fn eqIgnoreCase(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isRfc5987AttrChar(c: u8) -> bool {
    if ((c >= 'a' as u8 && c <= 'z' as u8) || (c >= 'A' as u8 && c <= 'Z' as u8) || (c >= '0' as u8 && c <= '9' as u8)) {
        return true;
    }
    return c == '!' as u8 || c == '#' as u8 || c == '$' as u8 || c == '&' as u8
        || c == '+' as u8 || c == '-' as u8 || c == '.' as u8 || c == '^' as u8
        || c == '_' as u8 || c == '`' as u8 || c == '|' as u8 || c == '~' as u8;
}

fn hexUpper(nib: u8) -> u8 {
    if (nib < 10) {
        return ('0' as u8) + nib;
    }
    return ('A' as u8) + (nib - 10);
}

fn fromHexDigit(c: u8) -> i32 {
    if (c >= '0' as u8 && c <= '9' as u8) {
        return (c - ('0' as u8)) as i32;
    }
    if (c >= 'a' as u8 && c <= 'f' as u8) {
        return (c - ('a' as u8) + 10) as i32;
    }
    if (c >= 'A' as u8 && c <= 'F' as u8) {
        return (c - ('A' as u8) + 10) as i32;
    }
    return -1;
}

fn appendSlice(dst: []u8, pos: usize, src: []const u8) -> i64 {
    if (src.len > dst.len - pos) {
        return -1;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[pos + i] = src[i];
        i += 1;
    }
    return (pos + src.len) as i64;
}

fn appendByte(dst: []u8, pos: usize, b: u8) -> i64 {
    if (pos >= dst.len) {
        return -1;
    }
    dst[pos] = b;
    return (pos + 1) as i64;
}

pub fn format(kind: u8, filename: []const u8, dst: []u8) -> i64 {
    let p: i64 = 0;
    if (kind == DISP_INLINE) {
        p = appendSlice(dst, 0, "inline");
    } else if (kind == DISP_ATTACHMENT) {
        p = appendSlice(dst, 0, "attachment");
    } else if (kind == DISP_FORM_DATA) {
        p = appendSlice(dst, 0, "form-data");
    } else {
        return -1;
    }
    if (p < 0) {
        return -1;
    }

    if (filename.len == 0) {
        return p;
    }

    // Strip any directory path prefix ('/' or '\') to prevent path traversal
    let base_start: usize = 0;
    let i: usize = 0;
    while (i < filename.len) {
        if (filename[i] == '/' as u8 || filename[i] == '\\' as u8) {
            base_start = i + 1;
        }
        i += 1;
    }
    if (base_start >= filename.len) {
        return p;
    }
    const base: []const u8 = filename[base_start..filename.len];

    // Check if any non-ASCII or control characters exist
    let has_non_ascii: bool = false;
    i = 0;
    while (i < base.len) {
        if (base[i] < 0x20 || base[i] >= 0x7F) {
            has_non_ascii = true;
            break;
        }
        i += 1;
    }

    p = appendSlice(dst, p as usize, "; filename=\"");
    if (p < 0) {
        return -1;
    }

    i = 0;
    while (i < base.len) {
        const c: u8 = base[i];
        if (c < 0x20 || c >= 0x7F) {
            p = appendByte(dst, p as usize, '?' as u8);
        } else if (c == '"' as u8 || c == '\\' as u8) {
            p = appendByte(dst, p as usize, '\\' as u8);
            if (p < 0) {
                return -1;
            }
            p = appendByte(dst, p as usize, c);
        } else {
            p = appendByte(dst, p as usize, c);
        }
        if (p < 0) {
            return -1;
        }
        i += 1;
    }

    p = appendByte(dst, p as usize, '"' as u8);
    if (p < 0) {
        return -1;
    }

    if (has_non_ascii) {
        p = appendSlice(dst, p as usize, "; filename*=UTF-8''");
        if (p < 0) {
            return -1;
        }
        i = 0;
        while (i < base.len) {
            const c: u8 = base[i];
            if (isRfc5987AttrChar(c)) {
                p = appendByte(dst, p as usize, c);
                if (p < 0) {
                    return -1;
                }
            } else {
                p = appendByte(dst, p as usize, '%' as u8);
                if (p < 0) {
                    return -1;
                }
                p = appendByte(dst, p as usize, hexUpper((c >> 4) & 0x0F));
                if (p < 0) {
                    return -1;
                }
                p = appendByte(dst, p as usize, hexUpper(c & 0x0F));
                if (p < 0) {
                    return -1;
                }
            }
            i += 1;
        }
    }

    return p;
}

fn decodePercent(src: []const u8, dst: []u8) -> i64 {
    let out_pos: usize = 0;
    let i: usize = 0;
    while (i < src.len) {
        if (src[i] == '%' as u8) {
            if (i + 2 >= src.len) {
                return -1;
            }
            const hi: i32 = fromHexDigit(src[i + 1]);
            const lo: i32 = fromHexDigit(src[i + 2]);
            if (hi < 0 || lo < 0) {
                return -1;
            }
            if (out_pos >= dst.len) {
                return -1;
            }
            dst[out_pos] = (((hi as u32) << 4) | (lo as u32)) as u8;
            out_pos += 1;
            i += 3;
        } else {
            if (out_pos >= dst.len) {
                return -1;
            }
            dst[out_pos] = src[i];
            out_pos += 1;
            i += 1;
        }
    }
    return out_pos as i64;
}

fn decodeQuotedOrToken(val: []const u8, dst: []u8) -> i64 {
    if (val.len == 0) {
        return 0;
    }
    let out_pos: usize = 0;
    if (val[0] == '"' as u8) {
        if (val.len < 2 || val[val.len - 1] != '"' as u8) {
            return -1;
        }
        let i: usize = 1;
        const limit: usize = val.len - 1;
        while (i < limit) {
            if (val[i] == '\\' as u8 && i + 1 < limit) {
                i += 1;
            }
            if (out_pos >= dst.len) {
                return -1;
            }
            dst[out_pos] = val[i];
            out_pos += 1;
            i += 1;
        }
        return out_pos as i64;
    }
    if (val.len > dst.len) {
        return -1;
    }
    let i: usize = 0;
    while (i < val.len) {
        dst[out_pos] = val[i];
        out_pos += 1;
        i += 1;
    }
    return out_pos as i64;
}

pub fn parse(header: []const u8, filename_out: []u8) -> ParsedDisposition {
    const fail = ParsedDisposition {
        kind: DISP_INVALID,
        filename_len: 0,
        is_utf8: false,
        ok: false,
    };

    let pos: usize = 0;
    while (pos < header.len && isWs(header[pos])) {
        pos += 1;
    }
    if (pos >= header.len) {
        return fail;
    }

    // Read disposition type up to ';'
    let type_end: usize = pos;
    while (type_end < header.len && header[type_end] != ';' as u8) {
        type_end += 1;
    }
    let trimmed_end: usize = type_end;
    while (trimmed_end > pos && isWs(header[trimmed_end - 1])) {
        trimmed_end -= 1;
    }
    const type_slice: []const u8 = header[pos..trimmed_end];

    let kind: u8 = DISP_INVALID;
    if (eqIgnoreCase(type_slice, "inline")) {
        kind = DISP_INLINE;
    } else if (eqIgnoreCase(type_slice, "attachment")) {
        kind = DISP_ATTACHMENT;
    } else if (eqIgnoreCase(type_slice, "form-data")) {
        kind = DISP_FORM_DATA;
    } else {
        return fail;
    }

    pos = type_end;
    if (pos < header.len && header[pos] == ';' as u8) {
        pos += 1;
    }

    let has_utf8: bool = false;
    let final_len: usize = 0;

    while (pos < header.len) {
        while (pos < header.len && isWs(header[pos])) {
            pos += 1;
        }
        if (pos >= header.len) {
            break;
        }

        // Scan parameter segment respecting quoted strings
        let seg_end: usize = pos;
        let in_quote: bool = false;
        while (seg_end < header.len) {
            const c: u8 = header[seg_end];
            if (in_quote) {
                if (c == '\\' as u8 && seg_end + 1 < header.len) {
                    seg_end += 2;
                    continue;
                }
                if (c == '"' as u8) {
                    in_quote = false;
                }
            } else {
                if (c == '"' as u8) {
                    in_quote = true;
                } else if (c == ';' as u8) {
                    break;
                }
            }
            seg_end += 1;
        }

        // Find '=' inside [pos, seg_end)
        let eq_pos: usize = pos;
        while (eq_pos < seg_end && header[eq_pos] != '=' as u8) {
            eq_pos += 1;
        }
        if (eq_pos < seg_end) {
            let k_end: usize = eq_pos;
            while (k_end > pos && isWs(header[k_end - 1])) {
                k_end -= 1;
            }
            let v_start: usize = eq_pos + 1;
            let v_end: usize = seg_end;
            while (v_start < v_end && isWs(header[v_start])) {
                v_start += 1;
            }
            while (v_end > v_start && isWs(header[v_end - 1])) {
                v_end -= 1;
            }

            const key_slice: []const u8 = header[pos..k_end];
            const val_slice: []const u8 = header[v_start..v_end];

            if (eqIgnoreCase(key_slice, "filename*")) {
                // RFC 5987 format: charset'language'value (e.g. UTF-8''...)
                let q1: usize = 0;
                while (q1 < val_slice.len && val_slice[q1] != '\'' as u8) {
                    q1 += 1;
                }
                if (q1 + 1 >= val_slice.len) {
                    return fail;
                }
                let q2: usize = q1 + 1;
                while (q2 < val_slice.len && val_slice[q2] != '\'' as u8) {
                    q2 += 1;
                }
                if (q2 >= val_slice.len) {
                    return fail;
                }
                const charset: []const u8 = val_slice[0..q1];
                if (!eqIgnoreCase(charset, "utf-8")) {
                    return fail;
                }
                const encoded_part: []const u8 = val_slice[(q2 + 1)..val_slice.len];
                const d_len: i64 = decodePercent(encoded_part, filename_out);
                if (d_len < 0) {
                    return fail;
                }
                has_utf8 = true;
                final_len = d_len as usize;
            } else if (eqIgnoreCase(key_slice, "filename") && !has_utf8) {
                const d_len: i64 = decodeQuotedOrToken(val_slice, filename_out);
                if (d_len < 0) {
                    return fail;
                }
                final_len = d_len as usize;
            }
        }

        pos = seg_end;
        if (pos < header.len && header[pos] == ';' as u8) {
            pos += 1;
        }
    }

    return ParsedDisposition {
        kind: kind,
        filename_len: final_len,
        is_utf8: has_utf8,
        ok: true,
    };
}
