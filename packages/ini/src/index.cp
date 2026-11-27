// @cap/ini — Zero-heap INI / .gitconfig / .npmrc / .editorconfig parser & writer
// MIT License (clean-room CP implementation)

import { eqIgnoreCase } from "cap:ascii";
import { bytesEq, findByteInBytes } from "cap:bin";
import { parseI64 } from "cap:fmt";

pub const MAX_ENTRIES: usize = 128;

pub struct IniEntry {
    pub section_start: usize,
    pub section_len: usize,
    pub key_start: usize,
    pub key_len: usize,
    pub val_start: usize,
    pub val_len: usize,
}

type IniVec16 = @Vector(16, u8);

fn iniBuildWsLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    lut[' ' as usize] = 1;
    lut['\t' as usize] = 1;
    return lut;
}

const INI_WS_LUT: [256]u8 = iniBuildWsLut();

fn isWs(c: u8) -> bool {
    return INI_WS_LUT[c as usize] != 0;
}

fn iniFindByte(buf: []const u8, target: u8, start: usize) -> usize {
    let i: usize = start;
    const len: usize = buf.len;
    const vt: IniVec16 = @splat(target);
    while (i + 16 <= len) {
        const v: IniVec16 = buf[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == vt)) {
            let k: usize = 0;
            while (k < 16) {
                if (buf[i + k] == target) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (buf[i] == target) {
            return i;
        }
        i += 1;
    }
    return len;
}

fn iniFindComment(buf: []const u8, start: usize) -> usize {
    let i: usize = start;
    const len: usize = buf.len;
    const v_semi: IniVec16 = @splat(';' as u8);
    const v_hash: IniVec16 = @splat('#' as u8);
    while (i + 16 <= len) {
        const v: IniVec16 = buf[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_semi) || @reduce(.Or, v == v_hash)) {
            let k: usize = 0;
            while (k < 16) {
                const b = buf[i + k];
                if (b == ';' as u8 || b == '#' as u8) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const b = buf[i];
        if (b == ';' as u8 || b == '#' as u8) {
            return i;
        }
        i += 1;
    }
    return len;
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
            const line_end: usize = iniFindByte(src, '\n' as u8, pos);
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
                const close: usize = iniFindByte(src[0..e], ']' as u8, s + 1);
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
            const eq: usize = iniFindByte(src[0..e], '=' as u8, s);
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
                    // Strip inline comment preceded by whitespace in unquoted value (single-pass SIMD)
                    let search_from: usize = v_start;
                    while (search_from < v_end) {
                        const next_cmt: usize = iniFindComment(src[0..v_end], search_from);
                        if (next_cmt >= v_end) {
                            break;
                        }
                        if (next_cmt == v_start || isWs(src[next_cmt - 1])) {
                            v_end = next_cmt;
                            break;
                        }
                        search_from = next_cmt + 1;
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
        return parseI64(v) ?? default_val;
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
        if (s.len > 0) {
            @memcpy(_.buf[_.pos..(_.pos + s.len)], s);
            _.pos += s.len;
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
        if (!needs_quote && iniFindComment(value, 0) < value.len) {
            needs_quote = true;
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
