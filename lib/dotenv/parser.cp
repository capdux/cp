// Source: /data/dev/bun/src/dotenv/
import std from "std";

pub const MAX_ENV_ENTRIES: usize = 256;
pub const ENV_BUFFER_CAPACITY: usize = 32768;

pub struct EnvEntry {
    pub key: []const u8,
    pub value: []const u8,
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

    pub fn get(key: []const u8): ([]const u8)? {
        let i: usize = _.count;
        while (i > 0) {
            i -= 1;
            if (std.mem.eql(u8, _.entries[i].key, key)) {
                return _.entries[i].value;
            }
        }
        return null;
    }

    pub fn getOr(key: []const u8, default_val: []const u8): []const u8 {
        if (val := _.get(key)) {
            return val;
        }
        return default_val;
    }

    pub fn has(key: []const u8): bool {
        return _.get(key) != null;
    }

    pub fn put(*_, key: []const u8, val: []const u8, overwrite: bool): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].key, key)) {
                if (overwrite) {
                    let stored_val = _.storeString(val);
                    _.entries[i].value = stored_val;
                    return true;
                }
                return false;
            }
            i += 1;
        }

        if (_.count >= MAX_ENV_ENTRIES) return false;

        let stored_key = _.storeString(key);
        let stored_val = _.storeString(val);
        _.entries[_.count] = EnvEntry{
            key: stored_key,
            value: stored_val,
        };
        _.count += 1;
        return true;
    }

    pub fn putUnescaped(*_, key: []const u8, raw_val: []const u8, overwrite: bool): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].key, key)) {
                if (overwrite) {
                    let stored_val = _.storeUnescaped(raw_val);
                    _.entries[i].value = stored_val;
                    return true;
                }
                return false;
            }
            i += 1;
        }

        if (_.count >= MAX_ENV_ENTRIES) return false;

        let stored_key = _.storeString(key);
        let stored_val = _.storeUnescaped(raw_val);
        _.entries[_.count] = EnvEntry{
            key: stored_key,
            value: stored_val,
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

    pub fn storeUnescaped(*_, raw: []const u8): []const u8 {
        let start = _.buf_len;
        let i: usize = 0;
        while (i < raw.len and _.buf_len < ENV_BUFFER_CAPACITY) {
            if (raw[i] == '\\' and i + 1 < raw.len) {
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
                    _.buffer[_.buf_len] = '$';
                    _.buf_len += 1;
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
    return (c >= 'a' and c <= 'z') or
        (c >= 'A' and c <= 'Z') or
        (c >= '0' and c <= '9') or
        c == '_' or c == '-' or c == '.';
}

pub fn isWhitespace(c: u8): bool {
    return c == ' ' or c == '\t';
}

pub struct ParsedValue {
    pub slice: []const u8,
    pub is_double_quoted: bool = false,
}

pub struct EnvParser {
    pub src: []const u8,
    pub pos: usize = 0,

    pub static fn init(src: []const u8): EnvParser {
        return EnvParser{
            src: src,
            pos: 0,
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
                * = map.putUnescaped(key, parsed_val.slice, overwrite);
            } else {
                * = map.put(key, parsed_val.slice, overwrite);
            }
        }

        if (expand) {
            let temp_buf: [4096]u8 = undefined;
            let i: usize = 0;
            while (i < map.count) {
                let cur_val = map.entries[i].value;
                let map_raw: *const anyopaque = @ptrCast(map);
                let expanded = expandString(&temp_buf, map_raw, cur_val);
                if (!std.mem.eql(u8, cur_val, expanded)) {
                    let stored = map.storeString(expanded);
                    map.entries[i].value = stored;
                }
                i += 1;
            }
        }
    }

    fn skipEmptyLinesAndComments(*_) {
        while (_.pos < _.src.len) {
            let c = _.src[_.pos];
            if (isWhitespace(c) or c == '\r' or c == '\n') {
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
        }
    }

    fn parseKey(*_): ([]const u8)? {
        while (_.pos < _.src.len and isWhitespace(_.src[_.pos])) {
            _.pos += 1;
        }
        if (_.pos >= _.src.len) return null;

        // Skip 'export ' prefix
        if (_.pos + 7 <= _.src.len) {
            let prefix = _.src[_.pos .. _.pos + 7];
            if (std.mem.eql(u8, prefix, "export ") or std.mem.eql(u8, prefix, "export\t")) {
                _.pos += 7;
                while (_.pos < _.src.len and isWhitespace(_.src[_.pos])) {
                    _.pos += 1;
                }
            }
        }

        let start = _.pos;
        while (_.pos < _.src.len and isKeyChar(_.src[_.pos])) {
            _.pos += 1;
        }
        let end = _.pos;
        if (start == end) return null;

        while (_.pos < _.src.len and isWhitespace(_.src[_.pos])) {
            _.pos += 1;
        }

        if (_.pos < _.src.len and (_.src[_.pos] == '=' or _.src[_.pos] == ':')) {
            _.pos += 1; // consume '=' or ':'
            return _.src[start..end];
        }

        return null;
    }

    fn parseValue(*_): ParsedValue {
        while (_.pos < _.src.len and isWhitespace(_.src[_.pos])) {
            _.pos += 1;
        }
        if (_.pos >= _.src.len or _.src[_.pos] == '\r' or _.src[_.pos] == '\n') {
            return ParsedValue{ slice: "", is_double_quoted: false };
        }

        let quote_char = _.src[_.pos];
        if (quote_char == '"') {
            return ParsedValue{ slice: _.parseDoubleQuoted(), is_double_quoted: true };
        } else if (quote_char == '\'') {
            return ParsedValue{ slice: _.parseSingleQuoted(), is_double_quoted: false };
        } else if (quote_char == '`') {
            return ParsedValue{ slice: _.parseBacktickQuoted(), is_double_quoted: false };
        } else {
            return ParsedValue{ slice: _.parseUnquoted(), is_double_quoted: false };
        }
    }

    fn parseDoubleQuoted(*_): []const u8 {
        _.pos += 1; // consume opening "
        let start = _.pos;
        let end = start;

        while (_.pos < _.src.len) {
            let c = _.src[_.pos];
            if (c == '\\') {
                _.pos += 2;
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
        while (_.pos < _.src.len and _.src[_.pos] != '\'') {
            _.pos += 1;
        }
        let end = _.pos;
        if (_.pos < _.src.len and _.src[_.pos] == '\'') {
            _.pos += 1; // consume closing '
        }
        return _.src[start..end];
    }

    fn parseBacktickQuoted(*_): []const u8 {
        _.pos += 1; // consume opening `
        let start = _.pos;
        while (_.pos < _.src.len and _.src[_.pos] != '`') {
            _.pos += 1;
        }
        let end = _.pos;
        if (_.pos < _.src.len and _.src[_.pos] == '`') {
            _.pos += 1; // consume closing `
        }
        return _.src[start..end];
    }

    fn parseUnquoted(*_): []const u8 {
        let start = _.pos;
        while (_.pos < _.src.len) {
            let c = _.src[_.pos];
            if (c == '\r' or c == '\n') break;
            if (c == '#' and _.pos > start and isWhitespace(_.src[_.pos - 1])) {
                break;
            }
            _.pos += 1;
        }
        let end = _.pos;

        while (end > start and isWhitespace(_.src[end - 1])) {
            end -= 1;
        }
        return _.src[start..end];
    }
}

pub fn expandString(out_buf: []u8, map_raw: *const anyopaque, val: []const u8): []const u8 {
    let map: *const EnvMap = @alignCast(@ptrCast(map_raw));
    let out_len: usize = 0;
    let i: usize = 0;
    while (i < val.len and out_len < out_buf.len) {
        if (val[i] == '\\' and i + 1 < val.len and val[i + 1] == '$') {
            out_buf[out_len] = '$';
            out_len += 1;
            i += 2;
            continue;
        }

        if (val[i] == '$' and i + 1 < val.len) {
            if (val[i + 1] == '{') {
                let start_key = i + 2;
                let end_key = start_key;
                let has_default = false;
                let start_default: usize = 0;
                let end_default: usize = 0;

                while (end_key < val.len and val[end_key] != '}') {
                    if (val[end_key] == ':' and end_key + 1 < val.len and val[end_key + 1] == '-') {
                        has_default = true;
                        start_default = end_key + 2;
                        break;
                    }
                    end_key += 1;
                }

                let key = val[start_key..end_key];
                if (has_default) {
                    end_default = start_default;
                    while (end_default < val.len and val[end_default] != '}') {
                        end_default += 1;
                    }
                }

                let default_val = if (has_default) val[start_default..end_default] else "";
                let lookup = map.get(key);
                let resolved = default_val;
                if (v := lookup) {
                    if (v.len > 0) {
                        resolved = v;
                    }
                }

                let k: usize = 0;
                while (k < resolved.len and out_len < out_buf.len) {
                    out_buf[out_len] = resolved[k];
                    out_len += 1;
                    k += 1;
                }

                let close_idx = if (has_default) end_default else end_key;
                if (close_idx < val.len and val[close_idx] == '}') {
                    i = close_idx + 1;
                } else {
                    i = close_idx;
                }
                continue;
            } else if (isKeyChar(val[i + 1])) {
                let start_key = i + 1;
                let end_key = start_key;
                while (end_key < val.len and isKeyChar(val[end_key])) {
                    end_key += 1;
                }
                let key = val[start_key..end_key];
                let resolved = map.getOr(key, "");
                let k: usize = 0;
                while (k < resolved.len and out_len < out_buf.len) {
                    out_buf[out_len] = resolved[k];
                    out_len += 1;
                    k += 1;
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

pub fn parse(content: []const u8): EnvMap {
    return parseWithOptions(content, true);
}

pub fn parseWithOptions(content: []const u8, expand: bool): EnvMap {
    let map = EnvMap.init();
    let parser = EnvParser.init(content);
    parser.parseAll(&map, true, expand);
    return map;
}
