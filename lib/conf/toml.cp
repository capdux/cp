// Source: Custom implementation by Antigravity AI
// lib/conf/toml.cp
// Lightweight zero-allocation TOML configuration parser
// Absorbed from Bun src/interchange/toml.cp

const std = @import("std");

pub struct TomlEntry {
    table: []const u8 = "";
    key: []const u8 = "";
    value: []const u8 = "";
}

pub struct TomlDoc {
    entries: [128]TomlEntry = undefined;
    count: usize = 0;

    pub static fn parse(content: []const u8): TomlDoc {
        let doc = TomlDoc {
            entries: undefined,
            count: 0,
        };
        doc.parseContent(content);
        return doc;
    }

    pub fn get(table: []const u8, key: []const u8): ([]const u8)? {
        let i: usize = _.count;
        while (i > 0) {
            i -= 1;
            if (std.mem.eql(u8, _.entries[i].table, table) && std.mem.eql(u8, _.entries[i].key, key)) {
                return _.entries[i].value;
            }
        }
        return null;
    }

    pub fn getRoot(key: []const u8): ([]const u8)? {
        return _.get("", key);
    }

    pub fn hasTable(table: []const u8): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].table, table)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn parseContent(*_, content: []const u8) {
        let current_table: []const u8 = "";
        let line_iter = std.mem.splitScalar(u8, content, '\n');

        while (raw_line := line_iter.next()) {
            const line = std.mem.trim(u8, raw_line, " \t\r");
            if (line.len == 0 || line[0] == '#') {
                continue;
            }

            if (line[0] == '[') {
                if (close_idx := std.mem.lastIndexOfScalar(u8, line, ']')) {
                    if (close_idx > 1) {
                        current_table = std.mem.trim(u8, line[1..close_idx], " \t[]");
                    }
                }
                continue;
            }

            if (eq_idx := std.mem.indexOfScalar(u8, line, '=')) {
                const raw_key = std.mem.trim(u8, line[0..eq_idx], " \t\"'");
                if (raw_key.len == 0) {
                    continue;
                }
                const raw_val = std.mem.trim(u8, line[eq_idx + 1 ..], " \t");
                const val = cleanTomlValue(raw_val);
                _.put(current_table, raw_key, val);
            }
        }
    }

    fn put(*_, table: []const u8, key: []const u8, value: []const u8) {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].table, table) && std.mem.eql(u8, _.entries[i].key, key)) {
                _.entries[i].value = value;
                return;
            }
            i += 1;
        }
        if (_.count < 128) {
            _.entries[_.count] = TomlEntry {
                table: table,
                key: key,
                value: value,
            };
            _.count += 1;
        }
    }
}

fn cleanTomlValue(raw: []const u8): []const u8 {
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

pub const TOML = TomlDoc;
