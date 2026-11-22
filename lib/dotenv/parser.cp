// Source: Absorbed from /data/dev/bun/src/dotenv/env_loader.cp | Synced: 2026-11-22T07:59:33Z
// lib/dotenv/parser.cp
// Zero-heap .env parser, quote unescaping, BOM/CRLF handling & variable expansion

import std from "std";

pub const MAX_ENV_ENTRIES: usize = 256;
pub const ENV_BUFFER_CAPACITY: usize = 32768;

pub struct EnvEntry {
    pub key: []const u8,
    pub value: []const u8,
    pub key_off: usize = 0,
    pub key_len: usize = 0,
    pub key_in_buf: bool = false,
    pub val_off: usize = 0,
    pub val_len: usize = 0,
    pub val_in_buf: bool = false,
}

pub struct EnvMap {
    pub entries: [MAX_ENV_ENTRIES]EnvEntry = undefined,
    pub count: usize = 0,
    pub buffer: [ENV_BUFFER_CAPACITY]u8 = undefined,
    pub buf_len: usize = 0,

    pub static fn init(): EnvMap {
        return EnvMap{
            entries: undefined,
            count: 0,
            buffer: undefined,
            buf_len: 0,
        };
    }

    pub static fn parse(content: []const u8): EnvMap {
        return parseWithOptions(content, true);
    }

    pub fn entryKey(idx: usize): []const u8 {
        const e = _.entries[idx];
        if (e.key_in_buf) {
            return _.buffer[e.key_off .. e.key_off + e.key_len];
        }
        return e.key;
    }

    pub fn entryValue(idx: usize): []const u8 {
        const e = _.entries[idx];
        if (e.val_in_buf) {
            return _.buffer[e.val_off .. e.val_off + e.val_len];
        }
        return e.value;
    }

    pub static fn getPtr(m: *const EnvMap, key: []const u8): ([]const u8)? {
        let i: usize = m.count;
        while (i > 0) {
            i -= 1;
            const e = m.entries[i];
            const k = if (e.key_in_buf) m.buffer[e.key_off .. e.key_off + e.key_len] else e.key;
            if (std.mem.eql(u8, k, key)) {
                if (e.val_in_buf) {
                    return m.buffer[e.val_off .. e.val_off + e.val_len];
                }
                return e.value;
            }
        }
        return null;
    }

    pub fn get(*_, key: []const u8): ([]const u8)? {
        return EnvMap.getPtr(_, key);
    }

    pub fn getOr(*_, key: []const u8, default_val: []const u8): []const u8 {
        if (v := EnvMap.getPtr(_, key)) {
            return v;
        }
        return default_val;
    }

    pub fn has(*_, key: []const u8): bool {
        return EnvMap.getPtr(_, key) != null;
    }

    pub fn put(*_, key: []const u8, val: []const u8, overwrite: bool): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entryKey(i), key)) {
                if (overwrite) {
                    const v_start = _.buf_len;
                    let stored_val = _.storeString(val);
                    const in_b = _.buf_len > v_start || val.len == 0;
                    _.entries[i].value = stored_val;
                    _.entries[i].val_off = v_start;
                    _.entries[i].val_len = stored_val.len;
                    _.entries[i].val_in_buf = in_b;
                    return true;
                }
                return false;
            }
            i += 1;
        }

        if (_.count >= MAX_ENV_ENTRIES) return false;

        const k_start = _.buf_len;
        let stored_key = _.storeString(key);
        const k_in_b = _.buf_len > k_start || key.len == 0;
        const v_start = _.buf_len;
        let stored_val = _.storeString(val);
        const v_in_b = _.buf_len > v_start || val.len == 0;
        _.entries[_.count] = EnvEntry{
            key: stored_key,
            value: stored_val,
            key_off: k_start,
            key_len: stored_key.len,
            key_in_buf: k_in_b,
            val_off: v_start,
            val_len: stored_val.len,
            val_in_buf: v_in_b,
        };
        _.count += 1;
        return true;
    }

    pub fn putNormalized(*_, key: []const u8, raw_val: []const u8, overwrite: bool): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entryKey(i), key)) {
                if (overwrite) {
                    const v_start = _.buf_len;
                    let stored_val = _.storeNormalizedNewlines(raw_val);
                    _.entries[i].value = stored_val;
                    _.entries[i].val_off = v_start;
                    _.entries[i].val_len = stored_val.len;
                    _.entries[i].val_in_buf = true;
                    return true;
                }
                return false;
            }
            i += 1;
        }

        if (_.count >= MAX_ENV_ENTRIES) return false;

        const k_start = _.buf_len;
        let stored_key = _.storeString(key);
        const k_in_b = _.buf_len > k_start || key.len == 0;
        const v_start = _.buf_len;
        let stored_val = _.storeNormalizedNewlines(raw_val);
        _.entries[_.count] = EnvEntry{
            key: stored_key,
            value: stored_val,
            key_off: k_start,
            key_len: stored_key.len,
            key_in_buf: k_in_b,
            val_off: v_start,
            val_len: stored_val.len,
            val_in_buf: true,
        };
        _.count += 1;
        return true;
    }

    pub fn putUnescaped(*_, key: []const u8, raw_val: []const u8, overwrite: bool): bool {
        return _.putUnescapedEx(key, raw_val, overwrite, true);
    }

    pub fn putUnescapedEx(*_, key: []const u8, raw_val: []const u8, overwrite: bool, preserve_dollar_escape: bool): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entryKey(i), key)) {
                if (overwrite) {
                    const v_start = _.buf_len;
                    let stored_val = _.storeUnescapedEx(raw_val, preserve_dollar_escape);
                    _.entries[i].value = stored_val;
                    _.entries[i].val_off = v_start;
                    _.entries[i].val_len = stored_val.len;
                    _.entries[i].val_in_buf = true;
                    return true;
                }
                return false;
            }
            i += 1;
        }

        if (_.count >= MAX_ENV_ENTRIES) return false;

        const k_start = _.buf_len;
        let stored_key = _.storeString(key);
        const k_in_b = _.buf_len > k_start || key.len == 0;
        const v_start = _.buf_len;
        let stored_val = _.storeUnescapedEx(raw_val, preserve_dollar_escape);
        _.entries[_.count] = EnvEntry{
            key: stored_key,
            value: stored_val,
            key_off: k_start,
            key_len: stored_key.len,
            key_in_buf: k_in_b,
            val_off: v_start,
            val_len: stored_val.len,
            val_in_buf: true,
        };
        _.count += 1;
        return true;
    }

    pub fn storeString(*_, s: []const u8): []const u8 {
        if (_.buf_len + s.len > ENV_BUFFER_CAPACITY) {
            return s;
        }
        let start = _.buf_len;
        let end = start + s.len;
        let i: usize = 0;
        while (i < s.len) {
            _.buffer[start + i] = s[i];
            i += 1;
        }
        _.buf_len = end;
        return _.buffer[start..end];
    }

    pub fn storeNormalizedNewlines(*_, raw: []const u8): []const u8 {
        let start = _.buf_len;
        let i: usize = 0;
        while (i < raw.len && _.buf_len < ENV_BUFFER_CAPACITY) {
            if (raw[i] == '\r') {
                if (i + 1 < raw.len && raw[i + 1] == '\n') {
                    _.buffer[_.buf_len] = '\n';
                    _.buf_len += 1;
                    i += 2;
                    continue;
                }
                _.buffer[_.buf_len] = '\n';
                _.buf_len += 1;
                i += 1;
                continue;
            }
            _.buffer[_.buf_len] = raw[i];
            _.buf_len += 1;
            i += 1;
        }
        return _.buffer[start.._.buf_len];
    }

    pub fn storeUnescaped(*_, raw: []const u8): []const u8 {
        return _.storeUnescapedEx(raw, true);
    }

    pub fn storeUnescapedEx(*_, raw: []const u8, preserve_dollar_escape: bool): []const u8 {
        let start = _.buf_len;
        let i: usize = 0;
        while (i < raw.len && _.buf_len < ENV_BUFFER_CAPACITY) {
            if (raw[i] == '\r') {
                if (i + 1 < raw.len && raw[i + 1] == '\n') {
                    _.buffer[_.buf_len] = '\n';
                    _.buf_len += 1;
                    i += 2;
                    continue;
                }
                _.buffer[_.buf_len] = '\n';
                _.buf_len += 1;
                i += 1;
                continue;
            }
            if (raw[i] == '\\' && i + 1 < raw.len) {
                let next = raw[i + 1];
                if (next == 'n') {
                    _.buffer[_.buf_len] = '\n';
                    _.buf_len += 1;
                    i += 2;
                    continue;
                } else if (next == 'r') {
                    _.buffer[_.buf_len] = '\r';
                    _.buf_len += 1;
                    i += 2;
                    continue;
                } else if (next == 't') {
                    _.buffer[_.buf_len] = '\t';
                    _.buf_len += 1;
                    i += 2;
                    continue;
                } else if (next == '"') {
                    _.buffer[_.buf_len] = '"';
                    _.buf_len += 1;
                    i += 2;
                    continue;
                } else if (next == '\\') {
                    _.buffer[_.buf_len] = '\\';
                    _.buf_len += 1;
                    i += 2;
                    continue;
                } else if (next == '$') {
                    if (preserve_dollar_escape) {
                        if (_.buf_len + 2 <= ENV_BUFFER_CAPACITY) {
                            _.buffer[_.buf_len] = '\\';
                            _.buffer[_.buf_len + 1] = '$';
                            _.buf_len += 2;
                        }
                    } else {
                        _.buffer[_.buf_len] = '$';
                        _.buf_len += 1;
                    }
                    i += 2;
                    continue;
                }
            }
            _.buffer[_.buf_len] = raw[i];
            _.buf_len += 1;
            i += 1;
        }
        return _.buffer[start.._.buf_len];
    }
}

pub fn isKeyChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '_' || c == '-' || c == '.';
}

pub fn isIdentStartChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_';
}

pub fn isIdentChar(c: u8): bool {
    return isIdentStartChar(c) || (c >= '0' && c <= '9');
}

pub fn isWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == 0x0B || c == 0x0C;
}

pub struct ParsedValue {
    pub slice: []const u8,
    pub is_double_quoted: bool = false,
    pub is_quoted: bool = false,
}

pub struct EnvParser {
    pub src: []const u8,
    pub pos: usize = 0,

    pub static fn init(src: []const u8): EnvParser {
        let start_pos: usize = 0;
        // Strip UTF-8 BOM (EF BB BF)
        if (src.len >= 3 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) {
            start_pos = 3;
        }
        return EnvParser{
            src: src,
            pos: start_pos,
        };
    }

    pub fn parseAll(*_, map: *EnvMap, overwrite: bool, expand: bool) {
        while (_.pos < _.src.len) {
            _.skipEmptyLinesAndComments();
            if (_.pos >= _.src.len) break;

            let maybe_key = _.parseKey();
            if (maybe_key == null) {
                _.skipToNextLine();
                continue;
            }
            let key = maybe_key.?;

            let parsed_val = _.parseValue();
            if (parsed_val.is_double_quoted) {
                * = map.putUnescapedEx(key, parsed_val.slice, overwrite, expand);
            } else if (parsed_val.is_quoted) {
                * = map.putNormalized(key, parsed_val.slice, overwrite);
            } else {
                * = map.put(key, parsed_val.slice, overwrite);
            }
        }

        if (expand) {
            let temp_buf: [4096]u8 = undefined;
            let i: usize = 0;
            while (i < map.count) {
                let cur_val = map.entryValue(i);
                let map_raw: *const anyopaque = unsafe { @ptrCast(map) };
                let expanded = expandString(&temp_buf, map_raw, cur_val);
                if (!std.mem.eql(u8, cur_val, expanded)) {
                    const v_start = map.buf_len;
                    let stored = map.storeString(expanded);
                    const in_b = map.buf_len > v_start || expanded.len == 0;
                    map.entries[i].value = stored;
                    map.entries[i].val_off = v_start;
                    map.entries[i].val_len = stored.len;
                    map.entries[i].val_in_buf = in_b;
                }
                i += 1;
            }
        }
    }

    fn utf8SpaceLen(*_): usize {
        if (_.pos >= _.src.len) return 0;
        const c = _.src[_.pos];
        if (isWhitespace(c)) return 1;
        // UTF-8 NBSP (U+00A0: C2 A0)
        if (_.pos + 1 < _.src.len && c == 0xC2 && _.src[_.pos + 1] == 0xA0) return 2;
        // UTF-8 Ideographic Space (U+3000: E3 80 80)
        if (_.pos + 2 < _.src.len && c == 0xE3 && _.src[_.pos + 1] == 0x80 && _.src[_.pos + 2] == 0x80) return 3;
        return 0;
    }

    fn skipHorizontalSpaces(*_) {
        while (_.pos < _.src.len) {
            const sp = _.utf8SpaceLen();
            if (sp == 0) break;
            _.pos += sp;
        }
    }

    fn skipEmptyLinesAndComments(*_) {
        while (_.pos < _.src.len) {
            const sp = _.utf8SpaceLen();
            if (sp > 0) {
                _.pos += sp;
                continue;
            }
            let c = _.src[_.pos];
            if (c == '\r' || c == '\n') {
                _.pos += 1;
                continue;
            }
            if (c == '#') {
                _.skipToNextLine();
                continue;
            }
            break;
        }
    }

    fn skipToNextLine(*_) {
        while (_.pos < _.src.len) {
            let c = _.src[_.pos];
            _.pos += 1;
            if (c == '\n') break;
            if (c == '\r') {
                if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                    _.pos += 1;
                }
                break;
            }
        }
    }

    fn skipTrailingCommentOnLine(*_) {
        _.skipHorizontalSpaces();
        if (_.pos < _.src.len && _.src[_.pos] == '#') {
            _.skipToNextLine();
            return;
        }
        if (_.pos < _.src.len && (_.src[_.pos] == '\r' || _.src[_.pos] == '\n')) {
            _.skipToNextLine();
            return;
        }
        if (_.pos < _.src.len) {
            _.skipToNextLine();
        }
    }

    fn parseKey(*_): ([]const u8)? {
        _.skipHorizontalSpaces();
        if (_.pos >= _.src.len) return null;

        // Skip 'export ' prefix (supports space, tab, or UTF-8 whitespace after 'export')
        if (_.pos + 6 < _.src.len && std.mem.eql(u8, _.src[_.pos .. _.pos + 6], "export")) {
            let after_exp = _.pos + 6;
            if (isWhitespace(_.src[after_exp])) {
                _.pos = after_exp;
                _.skipHorizontalSpaces();
            }
        }

        let start = _.pos;
        while (_.pos < _.src.len && isKeyChar(_.src[_.pos])) {
            _.pos += 1;
        }
        let end = _.pos;
        if (start == end) return null;

        _.skipHorizontalSpaces();

        if (_.pos < _.src.len && (_.src[_.pos] == '=' || _.src[_.pos] == ':')) {
            _.pos += 1; // consume '=' or ':'
            return _.src[start..end];
        }

        return null;
    }

    fn parseValue(*_): ParsedValue {
        _.skipHorizontalSpaces();
        if (_.pos >= _.src.len || _.src[_.pos] == '\r' || _.src[_.pos] == '\n') {
            return ParsedValue{ slice: "", is_double_quoted: false, is_quoted: false };
        }

        let quote_char = _.src[_.pos];
        if (quote_char == '"') {
            const s = _.parseDoubleQuoted();
            _.skipTrailingCommentOnLine();
            return ParsedValue{ slice: s, is_double_quoted: true, is_quoted: true };
        } else if (quote_char == '\'') {
            const s = _.parseSingleQuoted();
            _.skipTrailingCommentOnLine();
            return ParsedValue{ slice: s, is_double_quoted: false, is_quoted: true };
        } else if (quote_char == '`') {
            const s = _.parseBacktickQuoted();
            _.skipTrailingCommentOnLine();
            return ParsedValue{ slice: s, is_double_quoted: false, is_quoted: true };
        } else {
            return ParsedValue{ slice: _.parseUnquoted(), is_double_quoted: false, is_quoted: false };
        }
    }

    fn parseDoubleQuoted(*_): []const u8 {
        _.pos += 1; // consume opening "
        let start = _.pos;
        let end = start;

        while (_.pos < _.src.len) {
            let c = _.src[_.pos];
            if (c == '\\') {
                if (_.pos + 1 < _.src.len) {
                    _.pos += 2;
                    end = _.pos;
                } else {
                    _.pos += 1;
                    end = _.pos;
                }
                continue;
            }
            if (c == '"') {
                end = _.pos;
                _.pos += 1; // consume closing "
                break;
            }
            _.pos += 1;
            end = _.pos;
        }

        return _.src[start..end];
    }

    fn parseSingleQuoted(*_): []const u8 {
        _.pos += 1; // consume opening '
        let start = _.pos;
        while (_.pos < _.src.len && _.src[_.pos] != '\'') {
            _.pos += 1;
        }
        let end = _.pos;
        if (_.pos < _.src.len && _.src[_.pos] == '\'') {
            _.pos += 1; // consume closing '
        }
        return _.src[start..end];
    }

    fn parseBacktickQuoted(*_): []const u8 {
        _.pos += 1; // consume opening `
        let start = _.pos;
        while (_.pos < _.src.len && _.src[_.pos] != '`') {
            _.pos += 1;
        }
        let end = _.pos;
        if (_.pos < _.src.len && _.src[_.pos] == '`') {
            _.pos += 1; // consume closing `
        }
        return _.src[start..end];
    }

    fn parseUnquoted(*_): []const u8 {
        let start = _.pos;
        while (_.pos < _.src.len) {
            let c = _.src[_.pos];
            if (c == '\r' || c == '\n') break;
            if (c == '#' && (_.pos == start || isWhitespace(_.src[_.pos - 1]))) {
                _.skipToNextLine();
                break;
            }
            _.pos += 1;
        }
        let end = if (_.pos <= _.src.len) _.pos else _.src.len;
        // Note: if skipToNextLine ran due to '#', we need end before '#'
        // Re-scan end cleanly up to first unquoted '#' or newline starting from start
        let scan_end = start;
        while (scan_end < _.src.len) {
            const sc = _.src[scan_end];
            if (sc == '\r' || sc == '\n') break;
            if (sc == '#' && (scan_end == start || isWhitespace(_.src[scan_end - 1]))) {
                break;
            }
            scan_end += 1;
        }
        end = scan_end;

        while (end > start && isWhitespace(_.src[end - 1])) {
            end -= 1;
        }
        return _.src[start..end];
    }
}

fn expandStringWithDepth(out_buf: []u8, map_raw: *const anyopaque, val: []const u8, depth: usize): []const u8 {
    let map: *const EnvMap = unsafe { @alignCast(@ptrCast(map_raw)) };
    let out_len: usize = 0;
    let i: usize = 0;
    while (i < val.len && out_len < out_buf.len) {
        if (val[i] == '\\' && i + 1 < val.len && val[i + 1] == '$') {
            out_buf[out_len] = '$';
            out_len += 1;
            i += 2;
            continue;
        }

        if (val[i] == '$' && i + 1 < val.len) {
            if (val[i + 1] == '{') {
                let start_key = i + 2;
                let end_key = start_key;
                // mode: 0 = none, 1 = :- (unset or empty), 2 = - (unset only)
                let default_mode: u8 = 0;
                let start_default: usize = 0;
                let end_default: usize = 0;

                while (end_key < val.len && val[end_key] != '}') {
                    if (val[end_key] == ':' && end_key + 1 < val.len && val[end_key + 1] == '-') {
                        default_mode = 1;
                        start_default = end_key + 2;
                        break;
                    }
                    if (val[end_key] == '-' && end_key > start_key) {
                        default_mode = 2;
                        start_default = end_key + 1;
                        break;
                    }
                    end_key += 1;
                }

                let key = val[start_key..end_key];
                if (default_mode != 0) {
                    end_default = start_default;
                    let brace_depth: usize = 1;
                    while (end_default < val.len) {
                        if (val[end_default] == '\\' && end_default + 1 < val.len) {
                            end_default += 2;
                            continue;
                        }
                        if (val[end_default] == '{') {
                            brace_depth += 1;
                        } else if (val[end_default] == '}') {
                            brace_depth -= 1;
                            if (brace_depth == 0) break;
                        }
                        end_default += 1;
                    }
                }

                let close_idx = if (default_mode != 0) end_default else end_key;
                if (close_idx >= val.len || val[close_idx] != '}') {
                    // Unclosed "${" -> output literally
                    out_buf[out_len] = val[i];
                    out_len += 1;
                    i += 1;
                    continue;
                }

                let lookup = EnvMap.getPtr(map, key);
                let use_default = false;
                let val_slice: []const u8 = "";
                if (default_mode == 1) {
                    if (v := lookup) {
                        if (v.len > 0) {
                            val_slice = v;
                        } else {
                            use_default = true;
                        }
                    } else {
                        use_default = true;
                    }
                } else if (default_mode == 2) {
                    if (v := lookup) {
                        val_slice = v;
                    } else {
                        use_default = true;
                    }
                } else {
                    if (v := lookup) {
                        val_slice = v;
                    }
                }

                if (use_default) {
                    if (depth < 8 && std.mem.indexOfScalar(u8, val[start_default..end_default], '$') != null) {
                        let def_scratch: [1024]u8 = undefined;
                        const sub = expandStringWithDepth(&def_scratch, map_raw, val[start_default..end_default], depth + 1);
                        let k: usize = 0;
                        while (k < sub.len && out_len < out_buf.len) {
                            out_buf[out_len] = sub[k];
                            out_len += 1;
                            k += 1;
                        }
                    } else {
                        const raw_def = val[start_default..end_default];
                        let k: usize = 0;
                        while (k < raw_def.len && out_len < out_buf.len) {
                            out_buf[out_len] = raw_def[k];
                            out_len += 1;
                            k += 1;
                        }
                    }
                } else if (depth < 8 && std.mem.indexOfScalar(u8, val_slice, '$') != null) {
                    let val_scratch: [1024]u8 = undefined;
                    const sub = expandStringWithDepth(&val_scratch, map_raw, val_slice, depth + 1);
                    let k: usize = 0;
                    while (k < sub.len && out_len < out_buf.len) {
                        out_buf[out_len] = sub[k];
                        out_len += 1;
                        k += 1;
                    }
                } else {
                    let k: usize = 0;
                    while (k < val_slice.len && out_len < out_buf.len) {
                        out_buf[out_len] = val_slice[k];
                        out_len += 1;
                        k += 1;
                    }
                }

                i = close_idx + 1;
                continue;
            } else if (isIdentStartChar(val[i + 1])) {
                let start_key = i + 1;
                let end_key = start_key;
                while (end_key < val.len && isIdentChar(val[end_key])) {
                    end_key += 1;
                }
                let key = val[start_key..end_key];
                let resolved: []const u8 = "";
                if (v := EnvMap.getPtr(map, key)) {
                    resolved = v;
                }
                if (depth < 8 && std.mem.indexOfScalar(u8, resolved, '$') != null) {
                    let val_scratch: [1024]u8 = undefined;
                    const sub = expandStringWithDepth(&val_scratch, map_raw, resolved, depth + 1);
                    let k: usize = 0;
                    while (k < sub.len && out_len < out_buf.len) {
                        out_buf[out_len] = sub[k];
                        out_len += 1;
                        k += 1;
                    }
                } else {
                    let k: usize = 0;
                    while (k < resolved.len && out_len < out_buf.len) {
                        out_buf[out_len] = resolved[k];
                        out_len += 1;
                        k += 1;
                    }
                }
                i = end_key;
                continue;
            }
        }

        out_buf[out_len] = val[i];
        out_len += 1;
        i += 1;
    }
    return out_buf[0..out_len];
}

pub fn expandString(out_buf: []u8, map_raw: *const anyopaque, val: []const u8): []const u8 {
    return expandStringWithDepth(out_buf, map_raw, val, 0);
}

pub fn parse(content: []const u8): EnvMap {
    return parseWithOptions(content, true);
}

pub fn parseWithOptions(content: []const u8, expand: bool): EnvMap {
    let map = EnvMap.init();
    let parser = EnvParser.init(content);
    parser.parseAll(&map, true, expand);
    return map;
}
