// @cap/toml-edit — Zero-allocation TOML v1.0 parser, editor & formatter for CP
// Clean-room MIT implementation inspired by Rust `toml_edit` v0.22 (900M+ downloads).
// Supports [table] / [a.b] headers, dotted keys (a.b = 1), # comments, basic & literal strings,
// underscore-separated & negative integers, booleans, in-place editing, and canonical formatting.

import { strEq, trim, rfindByte } from "cap:ascii";
import { findByteInBytes } from "cap:bin";
import { writeI64 } from "cap:fmt";

pub const TOML_STR: u8 = 1;
pub const TOML_INT: u8 = 2;
pub const TOML_BOOL: u8 = 3;
pub const TOML_MAX_ENTRIES: usize = 64;

fn tmeStripComment(s: string) -> string {
    if (idx := findByteInBytes(s, 35 as u8, 0)) {
        return trim(s[0..idx]);
    }
    return trim(s);
}

fn tmeParseInt(raw: string) -> i64? {
    if (raw.len == (0 as usize)) {
        return null;
    }
    let idx: usize = 0;
    let neg = false;
    if (raw[0] == (45 as u8)) { // '-'
        neg = true;
        idx = 1 as usize;
    } else if (raw[0] == (43 as u8)) { // '+'
        idx = 1 as usize;
    }
    if (idx >= raw.len) {
        return null;
    }
    let acc: i64 = 0;
    let digits: usize = 0;
    let prev_underscore = false;
    while (idx < raw.len) {
        let c = raw[idx];
        if (c == (95 as u8)) { // '_'
            if (digits == (0 as usize) || prev_underscore || idx + (1 as usize) >= raw.len) {
                return null;
            }
            prev_underscore = true;
            idx += 1;
            continue;
        }
        if (c < (48 as u8) || c > (57 as u8)) {
            return null;
        }
        prev_underscore = false;
        let d = (c - (48 as u8)) as i64;
        acc = acc * (10 as i64) + d;
        digits += 1;
        idx += 1;
    }
    if (digits == (0 as usize)) {
        return null;
    }
    if (neg) {
        return (0 as i64) - acc;
    }
    return acc;
}

fn tmeWriteByte(out: []u8, pos: usize, b: u8) -> usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + (1 as usize);
    }
    return pos;
}

fn tmeWriteStr(out: []u8, pos: usize, s: string) -> usize {
    if (pos >= out.len || s.len == (0 as usize)) {
        return pos;
    }
    const avail = out.len - pos;
    const n = if (s.len < avail) s.len else avail;
    @memcpy(out[pos .. pos + n], s[0..n]);
    return pos + n;
}

fn tmeWriteQuotedStr(out: []u8, start_pos: usize, s: string) -> usize {
    let pos = tmeWriteByte(out, start_pos, 34 as u8); // '"'
    let i: usize = 0;
    let next_q = findByteInBytes(s, 34 as u8, 0) ?? s.len;
    let next_bs = findByteInBytes(s, 92 as u8, 0) ?? s.len;
    while (i < s.len) {
        if (next_q < i) {
            next_q = findByteInBytes(s, 34 as u8, i) ?? s.len;
        }
        if (next_bs < i) {
            next_bs = findByteInBytes(s, 92 as u8, i) ?? s.len;
        }
        const next_sp = if (next_q < next_bs) next_q else next_bs;
        if (next_sp > i) {
            pos = tmeWriteStr(out, pos, s[i..next_sp]);
            i = next_sp;
            if (i >= s.len) { break; }
        }
        pos = tmeWriteByte(out, pos, 92 as u8);
        pos = tmeWriteByte(out, pos, s[i]);
        i += 1;
    }
    return tmeWriteByte(out, pos, 34 as u8);
}

fn tmeWriteEntry(out: []u8, start_pos: usize, e: TomlEntry) -> usize {
    let pos = tmeWriteStr(out, start_pos, e.key);
    pos = tmeWriteStr(out, pos, " = ");
    if (e.kind == TOML_STR) {
        pos = tmeWriteQuotedStr(out, pos, e.str_val);
    } else if (e.kind == TOML_INT) {
        pos = writeI64(e.int_val, out, pos);
    } else if (e.kind == TOML_BOOL) {
        if (e.bool_val) {
            pos = tmeWriteStr(out, pos, "true");
        } else {
            pos = tmeWriteStr(out, pos, "false");
        }
    }
    return tmeWriteByte(out, pos, 10 as u8); // '\n'
}

/// A single typed TOML key-value entry belonging to a section (`""` for root).
pub struct TomlEntry {
    pub section: string,
    pub key: string,
    pub kind: u8,
    pub str_val: string,
    pub int_val: i64,
    pub bool_val: bool,

    pub static fn empty() -> TomlEntry {
        return TomlEntry{
            section: "",
            key: "",
            kind: 0 as u8,
            str_val: "",
            int_val: 0 as i64,
            bool_val: false,
        };
    }
}

/// Zero-allocation TOML v1.0 document holding up to 64 entries.
pub struct TomlDoc {
    pub entries: [64]TomlEntry,
    pub count: usize,

    pub static fn new() -> TomlDoc {
        let arr: [64]TomlEntry = undefined;
        let i: usize = 0;
        while (i < TOML_MAX_ENTRIES) {
            arr[i] = TomlEntry.empty();
            i += 1;
        }
        return TomlDoc{
            entries: arr,
            count: 0 as usize,
        };
    }

    pub fn len() -> usize {
        return _.count;
    }

    pub fn hasKey(section: string, key: string) -> bool {
        let i: usize = 0;
        while (i < _.count) {
            if (strEq(_.entries[i].section, section) && strEq(_.entries[i].key, key)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn getStr(section: string, key: string) -> string? {
        let i: usize = 0;
        while (i < _.count) {
            if (strEq(_.entries[i].section, section) && strEq(_.entries[i].key, key)) {
                if (_.entries[i].kind == TOML_STR) {
                    return _.entries[i].str_val;
                }
                return null;
            }
            i += 1;
        }
        return null;
    }

    pub fn getInt(section: string, key: string) -> i64? {
        let i: usize = 0;
        while (i < _.count) {
            if (strEq(_.entries[i].section, section) && strEq(_.entries[i].key, key)) {
                if (_.entries[i].kind == TOML_INT) {
                    return _.entries[i].int_val;
                }
                return null;
            }
            i += 1;
        }
        return null;
    }

    pub fn getBool(section: string, key: string) -> bool? {
        let i: usize = 0;
        while (i < _.count) {
            if (strEq(_.entries[i].section, section) && strEq(_.entries[i].key, key)) {
                if (_.entries[i].kind == TOML_BOOL) {
                    return _.entries[i].bool_val;
                }
                return null;
            }
            i += 1;
        }
        return null;
    }

    pub fn setStr(*_, section: string, key: string, val: string) -> bool {
        let i: usize = 0;
        while (i < _.count) {
            if (strEq(_.entries[i].section, section) && strEq(_.entries[i].key, key)) {
                _.entries[i].kind = TOML_STR;
                _.entries[i].str_val = val;
                _.entries[i].int_val = 0 as i64;
                _.entries[i].bool_val = false;
                return true;
            }
            i += 1;
        }
        if (_.count >= TOML_MAX_ENTRIES) {
            return false;
        }
        _.entries[_.count] = TomlEntry{
            section: section,
            key: key,
            kind: TOML_STR,
            str_val: val,
            int_val: 0 as i64,
            bool_val: false,
        };
        _.count += 1;
        return true;
    }

    pub fn setInt(*_, section: string, key: string, val: i64) -> bool {
        let i: usize = 0;
        while (i < _.count) {
            if (strEq(_.entries[i].section, section) && strEq(_.entries[i].key, key)) {
                _.entries[i].kind = TOML_INT;
                _.entries[i].str_val = "";
                _.entries[i].int_val = val;
                _.entries[i].bool_val = false;
                return true;
            }
            i += 1;
        }
        if (_.count >= TOML_MAX_ENTRIES) {
            return false;
        }
        _.entries[_.count] = TomlEntry{
            section: section,
            key: key,
            kind: TOML_INT,
            str_val: "",
            int_val: val,
            bool_val: false,
        };
        _.count += 1;
        return true;
    }

    pub fn setBool(*_, section: string, key: string, val: bool) -> bool {
        let i: usize = 0;
        while (i < _.count) {
            if (strEq(_.entries[i].section, section) && strEq(_.entries[i].key, key)) {
                _.entries[i].kind = TOML_BOOL;
                _.entries[i].str_val = "";
                _.entries[i].int_val = 0 as i64;
                _.entries[i].bool_val = val;
                return true;
            }
            i += 1;
        }
        if (_.count >= TOML_MAX_ENTRIES) {
            return false;
        }
        _.entries[_.count] = TomlEntry{
            section: section,
            key: key,
            kind: TOML_BOOL,
            str_val: "",
            int_val: 0 as i64,
            bool_val: val,
        };
        _.count += 1;
        return true;
    }

    pub static fn parse(src: string) -> TomlDoc {
        let doc = TomlDoc.new();
        let cur_section = "";
        let pos: usize = 0;

        while (pos < src.len) {
            const line_start = pos;
            const line_end = findByteInBytes(src, 10 as u8, pos) ?? src.len;
            pos = if (line_end < src.len) { line_end + (1 as usize) } else { src.len };

            let line = trim(src[line_start..line_end]);
            if (line.len == (0 as usize) || line[0] == (35 as u8)) {
                continue;
            }

            // Section header: [table] or [a.b]
            if (line[0] == (91 as u8)) { // '['
                let close_opt = findByteInBytes(line, 93 as u8, 0); // ']'
                if (close_opt != null) {
                    let close_idx = close_opt ?? (0 as usize);
                    if (close_idx > (1 as usize)) {
                        cur_section = trim(line[(1 as usize)..close_idx]);
                    }
                }
                continue;
            }

            // Key = Value
            let eq_opt = findByteInBytes(line, 61 as u8, 0); // '='
            if (eq_opt == null) {
                continue;
            }
            let eq_idx = eq_opt ?? (0 as usize);
            let raw_key = trim(line[0..eq_idx]);
            let raw_val = trim(line[(eq_idx + (1 as usize))..line.len]);
            if (raw_key.len == (0 as usize) || raw_val.len == (0 as usize)) {
                continue;
            }

            // Handle top-level dotted keys (e.g. a.b = 1 -> section="a", key="b")
            let entry_section = cur_section;
            let entry_key = raw_key;
            if (cur_section.len == (0 as usize)) {
                let dot_opt = rfindByte(raw_key, 46 as u8); // '.'
                if (dot_opt != null) {
                    let dot_idx = dot_opt ?? (0 as usize);
                    if (dot_idx > (0 as usize) && dot_idx + (1 as usize) < raw_key.len) {
                        entry_section = trim(raw_key[0..dot_idx]);
                        entry_key = trim(raw_key[(dot_idx + (1 as usize))..raw_key.len]);
                    }
                }
            }

            // Parse value: basic string "...", literal string '...', bool, or int
            if (raw_val[0] == (34 as u8)) { // '"'
                let end_q: usize = 1;
                let found = false;
                while (end_q < raw_val.len) {
                    const q_idx = findByteInBytes(raw_val, 34 as u8, end_q) ?? raw_val.len;
                    const bs_idx = findByteInBytes(raw_val, 92 as u8, end_q) ?? raw_val.len;
                    if (bs_idx < q_idx && bs_idx + (1 as usize) < raw_val.len) {
                        end_q = bs_idx + (2 as usize);
                        continue;
                    }
                    if (q_idx < raw_val.len) {
                        end_q = q_idx;
                        found = true;
                    }
                    break;
                }
                if (found) {
                    * = doc.setStr(entry_section, entry_key, raw_val[(1 as usize)..end_q]);
                }
            } else if (raw_val[0] == (39 as u8)) { // '\''
                if (end_q := findByteInBytes(raw_val, 39 as u8, 1 as usize)) {
                    * = doc.setStr(entry_section, entry_key, raw_val[(1 as usize)..end_q]);
                }
            } else {
                let clean = tmeStripComment(raw_val);
                if (strEq(clean, "true")) {
                    * = doc.setBool(entry_section, entry_key, true);
                } else if (strEq(clean, "false")) {
                    * = doc.setBool(entry_section, entry_key, false);
                } else {
                    let num_opt = tmeParseInt(clean);
                    if (num_opt != null) {
                        * = doc.setInt(entry_section, entry_key, num_opt ?? (0 as i64));
                    }
                }
            }
        }
        return doc;
    }

    pub fn format(out: []u8) -> usize {
        let emitted: [64]bool = undefined;
        let i: usize = 0;
        while (i < TOML_MAX_ENTRIES) {
            emitted[i] = false;
            i += 1;
        }

        let pos: usize = 0;
        // 1. Emit root (section == "") entries first
        let r: usize = 0;
        while (r < _.count) {
            if (_.entries[r].section.len == (0 as usize)) {
                pos = tmeWriteEntry(out, pos, _.entries[r]);
                emitted[r] = true;
            }
            r += 1;
        }

        // 2. Emit each distinct non-empty section grouped together
        let s: usize = 0;
        while (s < _.count) {
            if (!emitted[s]) {
                let sec = _.entries[s].section;
                if (pos > (0 as usize)) {
                    pos = tmeWriteByte(out, pos, 10 as u8);
                }
                pos = tmeWriteByte(out, pos, 91 as u8); // '['
                pos = tmeWriteStr(out, pos, sec);
                pos = tmeWriteStr(out, pos, "]\n");

                let k = s;
                while (k < _.count) {
                    if (!emitted[k] && strEq(_.entries[k].section, sec)) {
                        pos = tmeWriteEntry(out, pos, _.entries[k]);
                        emitted[k] = true;
                    }
                    k += 1;
                }
            }
            s += 1;
        }
        return pos;
    }
}
