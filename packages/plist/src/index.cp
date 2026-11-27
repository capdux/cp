// @cap/plist — Zero-allocation Apple XML Property List (.plist) parser & writer
// Inspired by crates.io:plist (https://github.com/ebarnard/rust-plist)

pub enum PlistValKind {
    String,
    Integer,
    Real,
    Bool,
    Data,
    Date,
    Invalid,
}

pub struct PlistValue {
    kind: PlistValKind,
    str_val: string,
    int_val: i64,
    bool_val: bool,
}

fn plstInvalid(): PlistValue {
    return PlistValue{
        kind: PlistValKind.Invalid,
        str_val: "",
        int_val: 0,
        bool_val: false,
    };
}

fn plstEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn plstStartsAt(hay: string, pos: usize, needle: string): bool {
    if (pos + needle.len > hay.len) { return false; }
    let i: usize = 0;
    while (i < needle.len) {
        if (hay[pos + i] != needle[i]) { return false; }
        i += 1;
    }
    return true;
}

fn plstFindSub(hay: string, start: usize, needle: string): usize? {
    if (needle.len == 0) { return start; }
    if (start + needle.len > hay.len) { return null; }
    let i: usize = start;
    const limit: usize = hay.len - needle.len;
    while (i <= limit) {
        if (plstStartsAt(hay, i, needle)) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn plstSkipWs(s: string, start: usize): usize {
    let i: usize = start;
    while (i < s.len) {
        const c = s[i];
        if (c == 32 || c == 9 || c == 10 || c == 13) {
            i += 1;
        } else {
            break;
        }
    }
    return i;
}

fn plstParseI64(s: string): i64 {
    let i: usize = plstSkipWs(s, 0);
    if (i >= s.len) { return 0; }
    let neg: bool = false;
    if (s[i] == 45) {
        neg = true;
        i += 1;
    } else if (s[i] == 43) {
        i += 1;
    }
    let acc: i64 = 0;
    while (i < s.len) {
        const c = s[i];
        if (c >= 48 && c <= 57) {
            const d: i64 = (c - 48) as i64;
            if (neg) {
                acc = acc * 10 - d;
            } else {
                acc = acc * 10 + d;
            }
            i += 1;
        } else {
            break;
        }
    }
    return acc;
}

fn plstSkipBalanced(xml: string, pos: usize, open_tag: string, close_tag: string): usize {
    let depth: usize = 1;
    let i: usize = pos + open_tag.len;
    while (i < xml.len && depth > 0) {
        if (plstStartsAt(xml, i, open_tag)) {
            depth += 1;
            i += open_tag.len;
        } else if (plstStartsAt(xml, i, close_tag)) {
            depth -= 1;
            i += close_tag.len;
        } else {
            i += 1;
        }
    }
    return i;
}

fn plstSkipElement(xml: string, pos: usize): usize {
    if (plstStartsAt(xml, pos, "<dict>")) {
        return plstSkipBalanced(xml, pos, "<dict>", "</dict>");
    }
    if (plstStartsAt(xml, pos, "<array>")) {
        return plstSkipBalanced(xml, pos, "<array>", "</array>");
    }
    // Self-closing or simple <tag>...</tag>
    if (gt := plstFindSub(xml, pos, ">")) {
        if (gt > pos && xml[gt - 1] == 47) {
            return gt + 1;
        }
        // Find matching closing tag </...>
        if (close_start := plstFindSub(xml, gt + 1, "</")) {
            if (close_end := plstFindSub(xml, close_start + 2, ">")) {
                return close_end + 1;
            }
        }
        return gt + 1;
    }
    return xml.len;
}

fn plstParseValueAt(xml: string, pos: usize): PlistValue {
    if (plstStartsAt(xml, pos, "<true/>") || plstStartsAt(xml, pos, "<true />")) {
        return PlistValue{
            kind: PlistValKind.Bool,
            str_val: "true",
            int_val: 1,
            bool_val: true,
        };
    }
    if (plstStartsAt(xml, pos, "<false/>") || plstStartsAt(xml, pos, "<false />")) {
        return PlistValue{
            kind: PlistValKind.Bool,
            str_val: "false",
            int_val: 0,
            bool_val: false,
        };
    }
    if (plstStartsAt(xml, pos, "<string/>") || plstStartsAt(xml, pos, "<string />")) {
        return PlistValue{
            kind: PlistValKind.String,
            str_val: "",
            int_val: 0,
            bool_val: false,
        };
    }
    if (plstStartsAt(xml, pos, "<string>")) {
        const start = pos + 8;
        if (end := plstFindSub(xml, start, "</string>")) {
            return PlistValue{
                kind: PlistValKind.String,
                str_val: xml[start..end],
                int_val: 0,
                bool_val: false,
            };
        }
        return plstInvalid();
    }
    if (plstStartsAt(xml, pos, "<integer>")) {
        const start = pos + 9;
        if (end := plstFindSub(xml, start, "</integer>")) {
            const raw = xml[start..end];
            return PlistValue{
                kind: PlistValKind.Integer,
                str_val: raw,
                int_val: plstParseI64(raw),
                bool_val: false,
            };
        }
        return plstInvalid();
    }
    if (plstStartsAt(xml, pos, "<real>")) {
        const start = pos + 6;
        if (end := plstFindSub(xml, start, "</real>")) {
            return PlistValue{
                kind: PlistValKind.Real,
                str_val: xml[start..end],
                int_val: 0,
                bool_val: false,
            };
        }
        return plstInvalid();
    }
    if (plstStartsAt(xml, pos, "<data>")) {
        const start = pos + 6;
        if (end := plstFindSub(xml, start, "</data>")) {
            return PlistValue{
                kind: PlistValKind.Data,
                str_val: xml[start..end],
                int_val: 0,
                bool_val: false,
            };
        }
        return plstInvalid();
    }
    if (plstStartsAt(xml, pos, "<date>")) {
        const start = pos + 6;
        if (end := plstFindSub(xml, start, "</date>")) {
            return PlistValue{
                kind: PlistValKind.Date,
                str_val: xml[start..end],
                int_val: 0,
                bool_val: false,
            };
        }
        return plstInvalid();
    }
    return plstInvalid();
}

pub fn plistGetDictValue(xml: string, key_name: string): PlistValue {
    let pos: usize = 0;
    if (dict_pos := plstFindSub(xml, 0, "<dict>")) {
        pos = dict_pos + 6;
    }

    while (pos < xml.len) {
        pos = plstSkipWs(xml, pos);
        if (pos >= xml.len) { break; }
        if (plstStartsAt(xml, pos, "</dict>")) {
            break;
        }
        if (plstStartsAt(xml, pos, "<!--")) {
            if (cend := plstFindSub(xml, pos + 4, "-->")) {
                pos = cend + 3;
                continue;
            }
            break;
        }
        if (plstStartsAt(xml, pos, "<key>")) {
            const kstart = pos + 5;
            if (kend := plstFindSub(xml, kstart, "</key>")) {
                const found_key = xml[kstart..kend];
                const vpos = plstSkipWs(xml, kend + 6);
                if (plstEq(found_key, key_name)) {
                    return plstParseValueAt(xml, vpos);
                }
                pos = plstSkipElement(xml, vpos);
                continue;
            }
            break;
        }
        pos += 1;
    }
    return plstInvalid();
}

fn plstWriteBytes(dst: []u8, pos: usize, src: string): usize {
    let i: usize = 0;
    while (i < src.len) {
        dst[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn plstFormatI64(val: i64, buf: []u8): usize {
    let tmp: [24]u8 = undefined;
    let len: usize = 0;
    if (val == 0) {
        if (buf.len < 1) { return 0; }
        buf[0] = 48;
        return 1;
    }
    let neg: bool = false;
    let u: u64 = 0;
    if (val < 0) {
        neg = true;
        // Handle i64::MIN safely via unsigned negation
        const raw_u: u64 = @as(u64, @bitCast(val));
        u = (~raw_u) +% 1;
    } else {
        u = val as u64;
    }
    while (u > 0) {
        const rem: u8 = (u % 10) as u8;
        tmp[len] = 48 + rem;
        len += 1;
        u = u / 10;
    }
    let total: usize = len;
    if (neg) { total += 1; }
    if (buf.len < total) { return 0; }
    let w: usize = 0;
    if (neg) {
        buf[w] = 45;
        w += 1;
    }
    let k: usize = len;
    while (k > 0) {
        k -= 1;
        buf[w] = tmp[k];
        w += 1;
    }
    return w;
}

pub fn plistWriteDictEntry(key: string, val: PlistValue, out: []u8): usize {
    if (val.kind == PlistValKind.Invalid) {
        return 0;
    }
    const key_part_len: usize = 5 + key.len + 6; // "<key>" + key + "</key>"

    if (val.kind == PlistValKind.Bool) {
        const tag = if (val.bool_val) "<true/>" else "<false/>";
        const needed = key_part_len + tag.len;
        if (out.len < needed) { return 0; }
        let p = plstWriteBytes(out, 0, "<key>");
        p = plstWriteBytes(out, p, key);
        p = plstWriteBytes(out, p, "</key>");
        p = plstWriteBytes(out, p, tag);
        return p;
    }

    if (val.kind == PlistValKind.Integer) {
        let num_buf: [24]u8 = undefined;
        const nlen = plstFormatI64(val.int_val, num_buf[0..24]);
        const needed = key_part_len + 9 + nlen + 10; // "<integer>" + digits + "</integer>"
        if (out.len < needed) { return 0; }
        let p = plstWriteBytes(out, 0, "<key>");
        p = plstWriteBytes(out, p, key);
        p = plstWriteBytes(out, p, "</key><integer>");
        p = plstWriteBytes(out, p, num_buf[0..nlen]);
        p = plstWriteBytes(out, p, "</integer>");
        return p;
    }

    let open_tag: string = "<string>";
    let close_tag: string = "</string>";
    if (val.kind == PlistValKind.Real) {
        open_tag = "<real>";
        close_tag = "</real>";
    } else if (val.kind == PlistValKind.Data) {
        open_tag = "<data>";
        close_tag = "</data>";
    } else if (val.kind == PlistValKind.Date) {
        open_tag = "<date>";
        close_tag = "</date>";
    }

    const needed = key_part_len + open_tag.len + val.str_val.len + close_tag.len;
    if (out.len < needed) { return 0; }
    let p = plstWriteBytes(out, 0, "<key>");
    p = plstWriteBytes(out, p, key);
    p = plstWriteBytes(out, p, "</key>");
    p = plstWriteBytes(out, p, open_tag);
    p = plstWriteBytes(out, p, val.str_val);
    p = plstWriteBytes(out, p, close_tag);
    return p;
}
