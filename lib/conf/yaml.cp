// Source: /data/dev/bun/src/interchange/yaml.cp
// lib/conf/yaml.cp
// Lightweight zero-allocation YAML key-value & block configuration parser
// Absorbed from /data/dev/bun/src/interchange/yaml.cp

const std = @import("std");

pub enum YamlChomp: u8 {
    Clip = 0,
    Strip = 1,
    Keep = 2,
}

pub struct YamlEntry {
    section: []const u8 = "";
    key: []const u8 = "";
    value: []const u8 = "";
}

pub struct YamlDoc {
    entries: [128]YamlEntry = undefined;
    count: usize = 0;

    pub static fn parse(content: []const u8): YamlDoc {
        let doc = YamlDoc {
            entries: undefined,
            count: 0,
        };
        doc.parseContent(content);
        return doc;
    }

    pub fn get(section: []const u8, key: []const u8): ([]const u8)? {
        let i: usize = _.count;
        while (i > 0) {
            i -= 1;
            if (std.mem.eql(u8, _.entries[i].section, section) && std.mem.eql(u8, _.entries[i].key, key)) {
                return _.entries[i].value;
            }
        }
        return null;
    }

    pub fn getRoot(key: []const u8): ([]const u8)? {
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

    pub fn getBool(section: []const u8, key: []const u8, default_val: bool): bool {
        if (v := _.get(section, key)) {
            const trimmed = std.mem.trim(u8, v, " \t\r");
            if (std.ascii.eqlIgnoreCase(trimmed, "true") || std.ascii.eqlIgnoreCase(trimmed, "yes") ||
                std.ascii.eqlIgnoreCase(trimmed, "on") || std.mem.eql(u8, trimmed, "1"))
            {
                return true;
            }
            if (std.ascii.eqlIgnoreCase(trimmed, "false") || std.ascii.eqlIgnoreCase(trimmed, "no") ||
                std.ascii.eqlIgnoreCase(trimmed, "off") || std.mem.eql(u8, trimmed, "0"))
            {
                return false;
            }
        }
        return default_val;
    }

    pub fn hasSection(section: []const u8): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].section, section)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn hasKey(section: []const u8, key: []const u8): bool {
        return _.get(section, key) != null;
    }

    fn parseContent(*_, content: []const u8) {
        let current_section: []const u8 = "";
        let line_iter = std.mem.splitScalar(u8, content, '\n');

        while (raw_line := line_iter.next()) {
            const trimmed_right = std.mem.trimEnd(u8, raw_line, " \t\r");
            if (trimmed_right.len == 0) {
                continue;
            }
            const trimmed = std.mem.trimStart(u8, trimmed_right, " \t");
            if (trimmed.len == 0 || trimmed[0] == '#' || std.mem.eql(u8, trimmed, "---") || std.mem.eql(u8, trimmed, "...")) {
                continue;
            }

            const indent = trimmed_right.len - trimmed.len;
            if (colon_idx := std.mem.indexOfScalar(u8, trimmed, ':')) {
                const key = std.mem.trim(u8, trimmed[0..colon_idx], " \t\"'");
                const rest = std.mem.trim(u8, trimmed[colon_idx + 1 ..], " \t");
                if (rest.len == 0 && indent == 0) {
                    current_section = key;
                } else {
                    const sec = if (indent == 0) "" else current_section;
                    const val = cleanYamlValue(rest);
                    _.put(sec, key, val);
                }
            }
        }
    }

    fn put(*_, section: []const u8, key: []const u8, value: []const u8) {
        if (_.count < 128) {
            _.entries[_.count] = YamlEntry {
                section: section,
                key: key,
                value: value,
            };
            _.count += 1;
        }
    }
}

fn cleanYamlValue(raw: []const u8): []const u8 {
    if (raw.len == 0) {
        return "";
    }
    if (raw.len >= 2 && ((raw[0] == '"' && raw[raw.len - 1] == '"') || (raw[0] == '\'' && raw[raw.len - 1] == '\''))) {
        return raw[1 .. raw.len - 1];
    }
    let end = raw.len;
    let i: usize = 0;
    while (i < raw.len) {
        if (raw[i] == '#' && i > 0 && (raw[i - 1] == ' ' || raw[i - 1] == '\t')) {
            end = i - 1;
            break;
        }
        i += 1;
    }
    return std.mem.trim(u8, raw[0..end], " \t\r");
}

pub const YAML = YamlDoc;
pub const yaml = YamlDoc;
