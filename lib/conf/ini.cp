// Source: Absorbed from /data/dev/bun/src/ini/ini.cp | Synced: 2026-11-22T07:47:14Z
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/ini/ini.cp
// Upstream Sync: Refer to /data/dev/bun/src/ini/ini.zig for future upstream updates.
// ============================================================================
// lib/conf/ini.cp
// Industrial-grade INI configuration parser, serializer, and section manager
// Absorbed from Bun src/ini/ini.cp & ini.zig

const std = @import("std");

pub struct Property {
    pub section: []const u8 = "";
    pub key: []const u8 = "";
    pub value: []const u8 = "";
}

pub struct Ini {
    pub props: [512]Property = undefined;
    pub count: usize = 0;

    pub static fn parse(content: []const u8): Ini {
        let res = Ini {
            props: undefined,
            count: 0,
        };
        res.parseContent(content);
        return res;
    }

    pub fn get(section: []const u8, key: []const u8): ([]const u8)? {
        let i: usize = _.count;
        while (i > 0) {
            i -= 1;
            if (std.mem.eql(u8, _.props[i].section, section) &&
                std.mem.eql(u8, _.props[i].key, key))
            {
                return _.props[i].value;
            }
        }
        return null;
    }

    pub fn getGlobal(key: []const u8): ([]const u8)? {
        return _.get("", key);
    }

    pub fn getOr(section: []const u8, key: []const u8, default_val: []const u8): []const u8 {
        if (v := _.get(section, key)) {
            return v;
        }
        return default_val;
    }

    pub fn getInt(section: []const u8, key: []const u8, default_val: i64): i64 {
        if (v := _.get(section, key)) {
            const trimmed = std.mem.trim(u8, v, " \t\r");
            if (trimmed.len > 0) {
                if (val := std.fmt.parseInt(i64, trimmed, 10) catch null) {
                    return val;
                }
            }
        }
        return default_val;
    }

    pub fn getFloat(section: []const u8, key: []const u8, default_val: f64): f64 {
        if (v := _.get(section, key)) {
            const trimmed = std.mem.trim(u8, v, " \t\r");
            if (trimmed.len > 0) {
                if (val := std.fmt.parseFloat(f64, trimmed) catch null) {
                    return val;
                }
            }
        }
        return default_val;
    }

    pub fn getBool(section: []const u8, key: []const u8, default_val: bool): bool {
        if (v := _.get(section, key)) {
            const trimmed = std.mem.trim(u8, v, " \t\r");
            if (std.mem.eql(u8, trimmed, "true") || std.mem.eql(u8, trimmed, "1") ||
                std.mem.eql(u8, trimmed, "yes") || std.mem.eql(u8, trimmed, "on"))
            {
                return true;
            }
            if (std.mem.eql(u8, trimmed, "false") || std.mem.eql(u8, trimmed, "0") ||
                std.mem.eql(u8, trimmed, "no") || std.mem.eql(u8, trimmed, "off"))
            {
                return false;
            }
        }
        return default_val;
    }

    pub fn hasSection(section: []const u8): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.props[i].section, section)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn hasKey(section: []const u8, key: []const u8): bool {
        return _.get(section, key) != null;
    }

    pub fn set(*_, section: []const u8, key: []const u8, value: []const u8) {
        _.put(section, key, value);
    }

    pub fn setGlobal(*_, key: []const u8, value: []const u8) {
        _.put("", key, value);
    }

    pub fn remove(*_, section: []const u8, key: []const u8): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.props[i].section, section) && std.mem.eql(u8, _.props[i].key, key)) {
                let j = i;
                while (j + 1 < _.count) {
                    _.props[j] = _.props[j + 1];
                    j += 1;
                }
                _.count -= 1;
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn removeSection(*_, section: []const u8): bool {
        let removed: bool = false;
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.props[i].section, section)) {
                let j = i;
                while (j + 1 < _.count) {
                    _.props[j] = _.props[j + 1];
                    j += 1;
                }
                _.count -= 1;
                removed = true;
            } else {
                i += 1;
            }
        }
        return removed;
    }

    pub fn getSections(*_): [][]const u8 {
        let buf = std.heap.page_allocator.alloc(string, 64) catch unreachable;
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            const sec = _.props[i].section;
            if (sec.len > 0) {
                let found = false;
                let k: usize = 0;
                while (k < n) {
                    if (std.mem.eql(u8, buf[k], sec)) {
                        found = true;
                        break;
                    }
                    k += 1;
                }
                if (!found && n < 64) {
                    buf[n] = sec;
                    n += 1;
                }
            }
            i += 1;
        }
        return buf[0..n];
    }

    pub fn getKeys(*_, section: []const u8): [][]const u8 {
        let buf = std.heap.page_allocator.alloc(string, 128) catch unreachable;
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.props[i].section, section) && n < 128) {
                buf[n] = _.props[i].key;
                n += 1;
            }
            i += 1;
        }
        return buf[0..n];
    }

    fn parseContent(*_, content: []const u8) {
        let current_section: []const u8 = "";
        let line_iter = std.mem.splitScalar(u8, content, '\n');

        while (raw_line := line_iter.next()) {
            const line = std.mem.trim(u8, raw_line, " \t\r");
            if (line.len == 0 || line[0] == ';' || line[0] == '#') {
                continue;
            }

            // Section header: [section_name] or [section.nested]
            if (line[0] == '[') {
                if (close_idx := std.mem.lastIndexOfScalar(u8, line, ']')) {
                    if (close_idx > 1) {
                        current_section = std.mem.trim(u8, line[1..close_idx], " \t");
                    }
                }
                continue;
            }

            // Key-value pair (delimiters = or :)
            let delim_idx: usize? = null;
            let i: usize = 0;
            while (i < line.len) {
                if (line[i] == '=' || line[i] == ':') {
                    delim_idx = i;
                    break;
                }
                i += 1;
            }

            if (delim_idx == null) {
                continue;
            }
            const idx = delim_idx.?;

            const raw_key = std.mem.trim(u8, line[0..idx], " \t");
            if (raw_key.len == 0) {
                continue;
            }

            const raw_val = std.mem.trim(u8, line[idx + 1 ..], " \t");
            const val = Ini.parseValue(raw_val);

            _.put(current_section, raw_key, val);
        }
    }

    static fn getEnv(name: []const u8): ([]const u8)? {
        let name_z = std.heap.page_allocator.allocSentinel(u8, name.len, 0) catch return null;
        defer std.heap.page_allocator.free(name_z);
        let i: usize = 0;
        while (i < name.len) {
            name_z[i] = name[i];
            i += 1;
        }
        if (p := std.c.getenv(name_z.ptr)) {
            return std.mem.span(p);
        }
        return null;
    }

    static fn expandEnv(raw: []const u8): []const u8 {
        if (std.mem.indexOfScalar(u8, raw, '$') == null) {
            return raw;
        }
        let buf = std.heap.page_allocator.alloc(u8, raw.len * 2 + 64) catch unreachable;
        let out_idx: usize = 0;
        let i: usize = 0;
        while (i < raw.len) {
            if (raw[i] == '$' && i + 1 < raw.len) {
                if (raw[i + 1] == '{') {
                    if (close_idx := std.mem.indexOfScalar(u8, raw[i + 2 ..], '}')) {
                        const var_name = raw[i + 2 .. i + 2 + close_idx];
                        if (val := Ini.getEnv(var_name)) {
                            let k: usize = 0;
                            while (k < val.len) {
                                buf[out_idx] = val[k];
                                out_idx += 1;
                                k += 1;
                            }
                        }
                        i = i + 2 + close_idx + 1;
                        continue;
                    }
                }
            }
            buf[out_idx] = raw[i];
            out_idx += 1;
            i += 1;
        }
        return buf[0..out_idx];
    }

    static fn unescapeStr(s: []const u8): []const u8 {
        if (std.mem.indexOfScalar(u8, s, '\\') == null) {
            return s;
        }
        let buf = std.heap.page_allocator.alloc(u8, s.len) catch unreachable;
        let out_idx: usize = 0;
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] == '\\' && i + 1 < s.len) {
                const next_ch = s[i + 1];
                if (next_ch == 'n') {
                    buf[out_idx] = '\n';
                    out_idx += 1;
                    i += 2;
                    continue;
                } else if (next_ch == 'r') {
                    buf[out_idx] = '\r';
                    out_idx += 1;
                    i += 2;
                    continue;
                } else if (next_ch == 't') {
                    buf[out_idx] = '\t';
                    out_idx += 1;
                    i += 2;
                    continue;
                } else if (next_ch == '\\') {
                    buf[out_idx] = '\\';
                    out_idx += 1;
                    i += 2;
                    continue;
                } else if (next_ch == '"') {
                    buf[out_idx] = '"';
                    out_idx += 1;
                    i += 2;
                    continue;
                } else if (next_ch == '\'') {
                    buf[out_idx] = '\'';
                    out_idx += 1;
                    i += 2;
                    continue;
                }
            }
            buf[out_idx] = s[i];
            out_idx += 1;
            i += 1;
        }
        return buf[0..out_idx];
    }

    static fn parseValue(raw: []const u8): []const u8 {
        if (raw.len == 0) {
            return "";
        }

        // Double quoted: unescape and expand
        if (raw.len >= 2 && raw[0] == '"' && raw[raw.len - 1] == '"') {
            const inner = raw[1 .. raw.len - 1];
            return Ini.expandEnv(Ini.unescapeStr(inner));
        }

        // Single quoted: raw literal
        if (raw.len >= 2 && raw[0] == '\'' && raw[raw.len - 1] == '\'') {
            return raw[1 .. raw.len - 1];
        }

        // Strip inline comments ; or # preceded by space/tab
        let end = raw.len;
        let i: usize = 0;
        while (i < raw.len) {
            if ((raw[i] == ';' || raw[i] == '#') && i > 0 && (raw[i - 1] == ' ' || raw[i - 1] == '\t')) {
                end = i - 1;
                break;
            }
            i += 1;
        }

        const trimmed = std.mem.trim(u8, raw[0..end], " \t\r");
        return Ini.expandEnv(trimmed);
    }

    fn put(*_, section: []const u8, key: []const u8, value: []const u8) {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.props[i].section, section) &&
                std.mem.eql(u8, _.props[i].key, key))
            {
                _.props[i].value = value;
                return;
            }
            i += 1;
        }

        if (_.count < _.props.len) {
            _.props[_.count] = Property {
                section: section,
                key: key,
                value: value,
            };
            _.count += 1;
        }
    }

    pub fn toString(): []const u8 {
        let buf = std.heap.page_allocator.alloc(u8, 32768) catch unreachable;
        let len: usize = 0;

        // 1. Global properties first
        let i: usize = 0;
        while (i < _.count) {
            if (_.props[i].section.len == 0) {
                const k = _.props[i].key;
                const v = _.props[i].value;
                let idx: usize = 0;
                while (idx < k.len) { buf[len] = k[idx]; len += 1; idx += 1; }
                buf[len] = ' '; len += 1;
                buf[len] = '='; len += 1;
                buf[len] = ' '; len += 1;
                idx = 0;
                while (idx < v.len) { buf[len] = v[idx]; len += 1; idx += 1; }
                buf[len] = '\n'; len += 1;
            }
            i += 1;
        }

        // 2. Sections
        let sec_buf = std.heap.page_allocator.alloc(string, 64) catch unreachable;
        let sec_count: usize = 0;
        i = 0;
        while (i < _.count) {
            const sec = _.props[i].section;
            if (sec.len > 0) {
                let found = false;
                let s_idx: usize = 0;
                while (s_idx < sec_count) {
                    if (std.mem.eql(u8, sec_buf[s_idx], sec)) {
                        found = true;
                        break;
                    }
                    s_idx += 1;
                }
                if (!found && sec_count < 64) {
                    sec_buf[sec_count] = sec;
                    sec_count += 1;
                }
            }
            i += 1;
        }

        let s: usize = 0;
        while (s < sec_count) {
            const sec_name = sec_buf[s];
            if (len > 0) {
                buf[len] = '\n'; len += 1;
            }
            buf[len] = '['; len += 1;
            let idx: usize = 0;
            while (idx < sec_name.len) { buf[len] = sec_name[idx]; len += 1; idx += 1; }
            buf[len] = ']'; len += 1;
            buf[len] = '\n'; len += 1;

            i = 0;
            while (i < _.count) {
                if (std.mem.eql(u8, _.props[i].section, sec_name)) {
                    const k = _.props[i].key;
                    const v = _.props[i].value;
                    idx = 0;
                    while (idx < k.len) { buf[len] = k[idx]; len += 1; idx += 1; }
                    buf[len] = ' '; len += 1;
                    buf[len] = '='; len += 1;
                    buf[len] = ' '; len += 1;
                    idx = 0;
                    while (idx < v.len) { buf[len] = v[idx]; len += 1; idx += 1; }
                    buf[len] = '\n'; len += 1;
                }
                i += 1;
            }
            s += 1;
        }

        return buf[0..len];
    }

    pub fn encode(): []const u8 {
        return _.toString();
    }
}

pub const ini = Ini;
