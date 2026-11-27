// @cap/ini — Zero-heap INI / .gitconfig / .npmrc / .editorconfig parser & writer
// MIT License (clean-room CP implementation)

pub const MAX_ENTRIES: usize = 128;

pub struct IniEntry {
    pub section_start: usize,
    pub section_len: usize,
    pub key_start: usize,
    pub key_len: usize,
    pub val_start: usize,
    pub val_len: usize,
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

fn bytesEq(a: []const u8, b: []const u8) -> bool {
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

pub struct IniDoc {
    pub entries: [128]IniEntry,
    pub count: usize,

    pub static fn parse(src: []const u8) -> IniDoc {
        let doc = IniDoc {
            entries: undefined,
            count: 0,
        };

        let cur_sec_start: usize = 0;
        let cur_sec_len: usize = 0;

        let pos: usize = 0;
        while (pos < src.len) {
            let line_end: usize = pos;
            while (line_end < src.len && src[line_end] != '\n' as u8) {
                line_end += 1;
            }
            let raw_end: usize = line_end;
            if (raw_end > pos && src[raw_end - 1] == '\r' as u8) {
                raw_end -= 1;
            }

            let s: usize = pos;
            let e: usize = raw_end;
            while (s < e && isWs(src[s])) {
                s += 1;
            }
            while (e > s && isWs(src[e - 1])) {
                e -= 1;
            }

            // Advance pos to next line
            pos = line_end;
            if (pos < src.len && src[pos] == '\n' as u8) {
                pos += 1;
            }

            if (s >= e) {
                continue;
            }
            const first: u8 = src[s];
            if (first == ';' as u8 || first == '#' as u8) {
                continue;
            }

            // Section header: [name]
            if (first == '[' as u8) {
                let close: usize = s + 1;
                while (close < e && src[close] != ']' as u8) {
                    close += 1;
                }
                if (close < e) {
                    let ns: usize = s + 1;
                    let ne: usize = close;
                    while (ns < ne && isWs(src[ns])) {
                        ns += 1;
                    }
                    while (ne > ns && isWs(src[ne - 1])) {
                        ne -= 1;
                    }
                    cur_sec_start = ns;
                    cur_sec_len = ne - ns;
                }
                continue;
            }

            // Key = Value
            let eq: usize = s;
            while (eq < e && src[eq] != '=' as u8) {
                eq += 1;
            }
            if (eq >= e) {
                continue;
            }

            let k_end: usize = eq;
            while (k_end > s && isWs(src[k_end - 1])) {
                k_end -= 1;
            }
            if (k_end <= s) {
                continue;
            }

            let v_start: usize = eq + 1;
            let v_end: usize = e;
            while (v_start < v_end && isWs(src[v_start])) {
                v_start += 1;
            }
            while (v_end > v_start && isWs(src[v_end - 1])) {
                v_end -= 1;
            }

            if (v_end - v_start >= 2) {
                const q: u8 = src[v_start];
                if ((q == '"' as u8 || q == '\'' as u8) && src[v_end - 1] == q) {
                    v_start += 1;
                    v_end -= 1;
                } else {
                    // Strip inline comment preceded by whitespace in unquoted value
                    let ci: usize = v_start;
                    while (ci < v_end) {
                        if ((src[ci] == ';' as u8 || src[ci] == '#' as u8) && (ci == v_start || isWs(src[ci - 1]))) {
                            v_end = ci;
                            break;
                        }
                        ci += 1;
                    }
                    while (v_end > v_start && isWs(src[v_end - 1])) {
                        v_end -= 1;
                    }
                }
            } else if (v_end > v_start && (src[v_start] == ';' as u8 || src[v_start] == '#' as u8)) {
                v_end = v_start;
            }

            if (doc.count < MAX_ENTRIES) {
                doc.entries[doc.count] = IniEntry {
                    section_start: cur_sec_start,
                    section_len: cur_sec_len,
                    key_start: s,
                    key_len: k_end - s,
                    val_start: v_start,
                    val_len: v_end - v_start,
                };
                doc.count += 1;
            }
        }

        return doc;
    }

    pub fn len() -> usize {
        return _.count;
    }

    fn findIndex(src: []const u8, section: []const u8, key: []const u8) -> i64 {
        let i: usize = _.count;
        while (i > 0) {
            i -= 1;
            const e: IniEntry = _.entries[i];
            const sec_slice: []const u8 = src[e.section_start..(e.section_start + e.section_len)];
            const key_slice: []const u8 = src[e.key_start..(e.key_start + e.key_len)];
            if (bytesEq(sec_slice, section) && bytesEq(key_slice, key)) {
                return i as i64;
            }
        }
        return -1;
    }

    pub fn has(src: []const u8, section: []const u8, key: []const u8) -> bool {
        return _.findIndex(src, section, key) >= 0;
    }

    pub fn get(src: []const u8, section: []const u8, key: []const u8) -> []const u8 {
        const idx: i64 = _.findIndex(src, section, key);
        if (idx < 0) {
            return "";
        }
        const e: IniEntry = _.entries[idx as usize];
        return src[e.val_start..(e.val_start + e.val_len)];
    }

    pub fn getBool(src: []const u8, section: []const u8, key: []const u8, default_val: bool) -> bool {
        const idx: i64 = _.findIndex(src, section, key);
        if (idx < 0) {
            return default_val;
        }
        const e: IniEntry = _.entries[idx as usize];
        const v: []const u8 = src[e.val_start..(e.val_start + e.val_len)];
        if (eqIgnoreCase(v, "true") || eqIgnoreCase(v, "yes") || eqIgnoreCase(v, "on") || bytesEq(v, "1")) {
            return true;
        }
        if (eqIgnoreCase(v, "false") || eqIgnoreCase(v, "no") || eqIgnoreCase(v, "off") || bytesEq(v, "0")) {
            return false;
        }
        return default_val;
    }

    pub fn getInt(src: []const u8, section: []const u8, key: []const u8, default_val: i64) -> i64 {
        const idx: i64 = _.findIndex(src, section, key);
        if (idx < 0) {
            return default_val;
        }
        const e: IniEntry = _.entries[idx as usize];
        const v: []const u8 = src[e.val_start..(e.val_start + e.val_len)];
        if (v.len == 0) {
            return default_val;
        }

        let i: usize = 0;
        let is_neg: bool = false;
        if (v[0] == '-' as u8) {
            is_neg = true;
            i = 1;
        } else if (v[0] == '+' as u8) {
            i = 1;
        }
        if (i >= v.len) {
            return default_val;
        }

        const max_pos: u64 = ((0x7FFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        const max_neg: u64 = (0x80000000 as u64) << 32;
        let max_mag: u64 = max_pos;
        if (is_neg) {
            max_mag = max_neg;
        }

        let acc: u64 = 0;
        while (i < v.len) {
            const c: u8 = v[i];
            if (c < '0' as u8 || c > '9' as u8) {
                return default_val;
            }
            const d: u64 = (c - ('0' as u8)) as u64;
            if (acc > (max_mag - d) / 10) {
                return default_val;
            }
            acc = acc * 10 + d;
            i += 1;
        }

        if (is_neg) {
            if (acc == max_neg) {
                return -(max_pos as i64) - 1;
            }
            return -(acc as i64);
        }
        return acc as i64;
    }
}

pub struct IniWriter {
    pub buf: []u8,
    pub pos: usize,
    pub overflow: bool,

    pub static fn init(buf: []u8) -> IniWriter {
        return IniWriter {
            buf: buf,
            pos: 0,
            overflow: false,
        };
    }

    fn writeByte(*_, b: u8) -> bool {
        if (_.overflow || _.pos >= _.buf.len) {
            _.overflow = true;
            return false;
        }
        _.buf[_.pos] = b;
        _.pos += 1;
        return true;
    }

    fn writeSlice(*_, s: []const u8) -> bool {
        if (_.overflow || s.len > _.buf.len - _.pos) {
            _.overflow = true;
            return false;
        }
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.pos] = s[i];
            _.pos += 1;
            i += 1;
        }
        return true;
    }

    pub fn writeSection(*_, name: []const u8) -> bool {
        if (!_.writeByte('[' as u8)) {
            return false;
        }
        if (!_.writeSlice(name)) {
            return false;
        }
        return _.writeSlice("]\n");
    }

    pub fn writeEntry(*_, key: []const u8, value: []const u8) -> bool {
        if (!_.writeSlice(key)) {
            return false;
        }
        if (!_.writeSlice(" = ")) {
            return false;
        }

        let needs_quote: bool = false;
        if (value.len > 0 && (isWs(value[0]) || isWs(value[value.len - 1]))) {
            needs_quote = true;
        }
        let i: usize = 0;
        while (i < value.len && !needs_quote) {
            const c: u8 = value[i];
            if (c == ';' as u8 || c == '#' as u8) {
                needs_quote = true;
            }
            i += 1;
        }

        if (needs_quote) {
            if (!_.writeByte('"' as u8)) {
                return false;
            }
            if (!_.writeSlice(value)) {
                return false;
            }
            if (!_.writeByte('"' as u8)) {
                return false;
            }
        } else {
            if (!_.writeSlice(value)) {
                return false;
            }
        }
        return _.writeByte('\n' as u8);
    }

    pub fn written() -> []const u8 {
        return _.buf[0.._.pos];
    }

    pub fn len() -> usize {
        return _.pos;
    }
}
