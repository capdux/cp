// lib/conf/ini.cp
// High-performance INI file parser in CP

const std = @import("std");

pub struct Property {
    section: []const u8 = "";
    key: []const u8 = "";
    value: []const u8 = "";
}

pub struct Ini {
    props: [128]Property = undefined;
    count: usize = 0;

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

    fn parseContent(*_, content: []const u8) {
        let current_section: []const u8 = "";
        let line_iter = std.mem.splitScalar(u8, content, '\n');

        while (raw_line := line_iter.next()) {
            const line = std.mem.trim(u8, raw_line, " \t\r");
            if (line.len == 0 || line[0] == ';' || line[0] == '#') {
                continue;
            }

            // Section header: [section_name]
            if (line[0] == '[') {
                if (close_idx := std.mem.lastIndexOfScalar(u8, line, ']')) {
                    if (close_idx > 1) {
                        current_section = std.mem.trim(u8, line[1..close_idx], " \t");
                    }
                }
                continue;
            }

            // Key-value pair
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

    static fn parseValue(raw: []const u8): []const u8 {
        if (raw.len == 0) {
            return "";
        }

        // Double quoted
        if (raw.len >= 2 && raw[0] == '"' && raw[raw.len - 1] == '"') {
            return raw[1 .. raw.len - 1];
        }

        // Single quoted
        if (raw.len >= 2 && raw[0] == '\'' && raw[raw.len - 1] == '\'') {
            return raw[1 .. raw.len - 1];
        }

        // Strip inline comments ; or #
        let end = raw.len;
        let i: usize = 0;
        while (i < raw.len) {
            if ((raw[i] == ';' || raw[i] == '#') && i > 0 && (raw[i - 1] == ' ' || raw[i - 1] == '\t')) {
                end = i - 1;
                break;
            }
            i += 1;
        }

        return std.mem.trim(u8, raw[0..end], " \t\r");
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
}

pub const IniParser = Ini;

pub const ini = struct {
    pub const parse = Ini.parse;
};
