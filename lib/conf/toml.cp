// Source: bun/src/interchange/toml.cp
// ============================================================================
// Absorbed from Bun: bun/src/interchange/toml.cp & bun/src/interchange/toml/lexer.cp
// Upstream Sync: Refer to bun/src/interchange/toml.zig for future upstream updates.
// ============================================================================
// lib/conf/toml.cp
// Industrial-grade TOML configuration parser, serializer, and document model
// Absorbed from Bun src/interchange/toml.cp & toml/lexer.cp

const std = @import("std");

pub struct TomlEntry {
    pub table: []const u8 = "";
    pub key: []const u8 = "";
    pub value: []const u8 = "";
}

pub struct TomlDoc {
    pub entries: [512]TomlEntry = undefined;
    pub count: usize = 0;

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
            if (std.mem.eql(u8, _.entries[i].table, table) &&
                std.mem.eql(u8, _.entries[i].key, key))
            {
                return _.entries[i].value;
            }
        }
        return null;
    }

    pub fn getRoot(key: []const u8): ([]const u8)? {
        return _.get("", key);
    }

    pub fn getOr(table: []const u8, key: []const u8, default_val: []const u8): []const u8 {
        if (v := _.get(table, key)) {
            return v;
        }
        return default_val;
    }

    pub fn getInt(table: []const u8, key: []const u8, default_val: i64): i64 {
        if (v := _.get(table, key)) {
            const clean = TomlDoc.stripUnderscores(v);
            if (clean.len > 2 && clean[0] == '0' && (clean[1] == 'x' || clean[1] == 'X')) {
                if (val := std.fmt.parseInt(i64, clean[2..], 16) catch null) {
                    return val;
                }
            } else if (clean.len > 2 && clean[0] == '0' && (clean[1] == 'o' || clean[1] == 'O')) {
                if (val := std.fmt.parseInt(i64, clean[2..], 8) catch null) {
                    return val;
                }
            } else if (clean.len > 2 && clean[0] == '0' && (clean[1] == 'b' || clean[1] == 'B')) {
                if (val := std.fmt.parseInt(i64, clean[2..], 2) catch null) {
                    return val;
                }
            } else {
                let to_parse = clean;
                if (clean.len > 0 && clean[0] == '+') {
                    to_parse = clean[1..];
                }
                if (val := std.fmt.parseInt(i64, to_parse, 10) catch null) {
                    return val;
                }
            }
        }
        return default_val;
    }

    pub fn getFloat(table: []const u8, key: []const u8, default_val: f64): f64 {
        if (v := _.get(table, key)) {
            const clean = TomlDoc.stripUnderscores(v);
            if (std.mem.eql(u8, clean, "inf") || std.mem.eql(u8, clean, "+inf")) {
                return std.math.inf(f64);
            }
            if (std.mem.eql(u8, clean, "-inf")) {
                return -1.0 * std.math.inf(f64);
            }
            if (std.mem.eql(u8, clean, "nan") || std.mem.eql(u8, clean, "+nan") || std.mem.eql(u8, clean, "-nan")) {
                return std.math.nan(f64);
            }
            let to_parse = clean;
            if (clean.len > 0 && clean[0] == '+') {
                to_parse = clean[1..];
            }
            if (val := std.fmt.parseFloat(f64, to_parse) catch null) {
                return val;
            }
        }
        return default_val;
    }

    pub fn getBool(table: []const u8, key: []const u8, default_val: bool): bool {
        if (v := _.get(table, key)) {
            if (std.mem.eql(u8, v, "true")) {
                return true;
            }
            if (std.mem.eql(u8, v, "false")) {
                return false;
            }
        }
        return default_val;
    }

    pub fn getArray(table: []const u8, key: []const u8): [][]const u8 {
        if (v := _.get(table, key)) {
            return TomlDoc.parseArrayElements(v);
        }
        let empty_buf = std.heap.page_allocator.alloc(string, 0) catch unreachable;
        return empty_buf;
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

    pub fn hasKey(table: []const u8, key: []const u8): bool {
        return _.get(table, key) != null;
    }

    pub fn set(*_, table: []const u8, key: []const u8, value: []const u8) {
        _.put(table, key, value);
    }

    pub fn setRoot(*_, key: []const u8, value: []const u8) {
        _.put("", key, value);
    }

    pub fn remove(*_, table: []const u8, key: []const u8): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].table, table) && std.mem.eql(u8, _.entries[i].key, key)) {
                let j = i;
                while (j + 1 < _.count) {
                    _.entries[j] = _.entries[j + 1];
                    j += 1;
                }
                _.count -= 1;
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn removeTable(*_, table: []const u8): bool {
        let removed = false;
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].table, table)) {
                let j = i;
                while (j + 1 < _.count) {
                    _.entries[j] = _.entries[j + 1];
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

    pub fn getTables(*_): [][]const u8 {
        let buf = std.heap.page_allocator.alloc(string, 64) catch unreachable;
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            const tbl = _.entries[i].table;
            if (tbl.len > 0) {
                let found = false;
                let k: usize = 0;
                while (k < n) {
                    if (std.mem.eql(u8, buf[k], tbl)) {
                        found = true;
                        break;
                    }
                    k += 1;
                }
                if (!found && n < 64) {
                    buf[n] = tbl;
                    n += 1;
                }
            }
            i += 1;
        }
        return buf[0..n];
    }

    pub fn getKeys(*_, table: []const u8): [][]const u8 {
        let buf = std.heap.page_allocator.alloc(string, 128) catch unreachable;
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].table, table) && n < 128) {
                buf[n] = _.entries[i].key;
                n += 1;
            }
            i += 1;
        }
        return buf[0..n];
    }

    fn parseContent(*_, content: []const u8) {
        let current_table: []const u8 = "";
        let line_iter = std.mem.splitScalar(u8, content, '\n');

        while (raw_line := line_iter.next()) {
            const line = std.mem.trim(u8, raw_line, " \t\r");
            if (line.len == 0 || line[0] == '#') {
                continue;
            }

            // Array of tables: [[table_name]]
            if (line.len >= 4 && line[0] == '[' && line[1] == '[') {
                if (close_idx := std.mem.lastIndexOfScalar(u8, line, ']')) {
                    if (close_idx > 2 && line[close_idx - 1] == ']') {
                        current_table = std.mem.trim(u8, line[2 .. close_idx - 1], " \t");
                    }
                }
                continue;
            }

            // Standard table: [table_name]
            if (line[0] == '[') {
                if (close_idx := std.mem.lastIndexOfScalar(u8, line, ']')) {
                    if (close_idx > 1) {
                        current_table = std.mem.trim(u8, line[1..close_idx], " \t");
                    }
                }
                continue;
            }

            // Key = Value
            if (eq_idx := std.mem.indexOfScalar(u8, line, '=')) {
                const raw_key = std.mem.trim(u8, line[0..eq_idx], " \t\"'");
                if (raw_key.len == 0) {
                    continue;
                }

                // Handle dotted keys in key itself: e.g. a.b.c = "foo"
                let target_table = current_table;
                let target_key = raw_key;

                if (dot_idx := std.mem.lastIndexOfScalar(u8, raw_key, '.')) {
                    if (dot_idx > 0 && dot_idx + 1 < raw_key.len) {
                        const prefix = raw_key[0..dot_idx];
                        target_key = raw_key[dot_idx + 1 ..];
                        if (current_table.len > 0) {
                            let comb = std.heap.page_allocator.alloc(u8, current_table.len + 1 + prefix.len) catch unreachable;
                            @memcpy(comb[0..current_table.len], current_table);
                            comb[current_table.len] = '.';
                            @memcpy(comb[current_table.len + 1 ..], prefix);
                            target_table = comb;
                        } else {
                            target_table = prefix;
                        }
                    }
                }

                const raw_val = std.mem.trim(u8, line[eq_idx + 1 ..], " \t");
                const val = TomlDoc.cleanTomlValue(raw_val);
                _.put(target_table, target_key, val);
            }
        }
    }

    static fn cleanTomlValue(raw: []const u8): []const u8 {
        if (raw.len == 0) {
            return "";
        }

        // Multi-line basic string: """..."""
        if (raw.len >= 6 && std.mem.startsWith(u8, raw, "\"\"\"") && std.mem.endsWith(u8, raw, "\"\"\"")) {
            const inner = raw[3 .. raw.len - 3];
            return TomlDoc.unescapeToml(inner);
        }

        // Multi-line literal string: '''...'''
        if (raw.len >= 6 && std.mem.startsWith(u8, raw, "'''") && std.mem.endsWith(u8, raw, "'''")) {
            return raw[3 .. raw.len - 3];
        }

        // Single line double-quoted: "..."
        if (raw.len >= 2 && raw[0] == '"' && raw[raw.len - 1] == '"') {
            const inner = raw[1 .. raw.len - 1];
            return TomlDoc.unescapeToml(inner);
        }

        // Single line literal: '...'
        if (raw.len >= 2 && raw[0] == '\'' && raw[raw.len - 1] == '\'') {
            return raw[1 .. raw.len - 1];
        }

        // Inline table or array: retain structure
        if ((raw[0] == '[' && raw[raw.len - 1] == ']') ||
            (raw[0] == '{' && raw[raw.len - 1] == '}'))
        {
            return raw;
        }

        // Strip inline comments #
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

    static fn unescapeToml(s: []const u8): []const u8 {
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
                }
            }
            buf[out_idx] = s[i];
            out_idx += 1;
            i += 1;
        }
        return buf[0..out_idx];
    }

    static fn stripUnderscores(s: []const u8): []const u8 {
        if (std.mem.indexOfScalar(u8, s, '_') == null) {
            return s;
        }
        let buf = std.heap.page_allocator.alloc(u8, s.len) catch unreachable;
        let out_idx: usize = 0;
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] != '_') {
                buf[out_idx] = s[i];
                out_idx += 1;
            }
            i += 1;
        }
        return buf[0..out_idx];
    }

    static fn parseArrayElements(raw: []const u8): [][]const u8 {
        const trimmed = std.mem.trim(u8, raw, " \t\r\n");
        if (trimmed.len < 2 || trimmed[0] != '[' || trimmed[trimmed.len - 1] != ']') {
            let empty = std.heap.page_allocator.alloc(string, 0) catch unreachable;
            return empty;
        }
        const inner = trimmed[1 .. trimmed.len - 1];
        let buf = std.heap.page_allocator.alloc(string, 64) catch unreachable;
        let count: usize = 0;

        let in_quotes = false;
        let quote_char: u8 = 0;
        let start: usize = 0;
        let i: usize = 0;

        while (i < inner.len) {
            const ch = inner[i];
            if (in_quotes) {
                if (ch == quote_char && (i == 0 || inner[i - 1] != '\\')) {
                    in_quotes = false;
                }
            } else {
                if (ch == '"' || ch == '\'') {
                    in_quotes = true;
                    quote_char = ch;
                } else if (ch == ',') {
                    const item = std.mem.trim(u8, inner[start..i], " \t\r\n");
                    if (item.len > 0 && count < 64) {
                        buf[count] = TomlDoc.cleanTomlValue(item);
                        count += 1;
                    }
                    start = i + 1;
                }
            }
            i += 1;
        }

        const last_item = std.mem.trim(u8, inner[start..], " \t\r\n");
        if (last_item.len > 0 && count < 64) {
            buf[count] = TomlDoc.cleanTomlValue(last_item);
            count += 1;
        }

        return buf[0..count];
    }

    fn put(*_, table: []const u8, key: []const u8, value: []const u8) {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].table, table) &&
                std.mem.eql(u8, _.entries[i].key, key))
            {
                _.entries[i].value = value;
                return;
            }
            i += 1;
        }
        if (_.count < _.entries.len) {
            _.entries[_.count] = TomlEntry {
                table: table,
                key: key,
                value: value,
            };
            _.count += 1;
        }
    }

    pub fn toString(): []const u8 {
        let buf = std.heap.page_allocator.alloc(u8, 32768) catch unreachable;
        let len: usize = 0;

        // 1. Root properties
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].table.len == 0) {
                const k = _.entries[i].key;
                const v = _.entries[i].value;
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

        // 2. Tables
        let tbl_buf = std.heap.page_allocator.alloc(string, 64) catch unreachable;
        let tbl_count: usize = 0;
        i = 0;
        while (i < _.count) {
            const tbl = _.entries[i].table;
            if (tbl.len > 0) {
                let found = false;
                let k: usize = 0;
                while (k < tbl_count) {
                    if (std.mem.eql(u8, tbl_buf[k], tbl)) {
                        found = true;
                        break;
                    }
                    k += 1;
                }
                if (!found && tbl_count < 64) {
                    tbl_buf[tbl_count] = tbl;
                    tbl_count += 1;
                }
            }
            i += 1;
        }

        let t: usize = 0;
        while (t < tbl_count) {
            const tbl_name = tbl_buf[t];
            if (len > 0) {
                buf[len] = '\n'; len += 1;
            }
            buf[len] = '['; len += 1;
            let idx: usize = 0;
            while (idx < tbl_name.len) { buf[len] = tbl_name[idx]; len += 1; idx += 1; }
            buf[len] = ']'; len += 1;
            buf[len] = '\n'; len += 1;

            i = 0;
            while (i < _.count) {
                if (std.mem.eql(u8, _.entries[i].table, tbl_name)) {
                    const k = _.entries[i].key;
                    const v = _.entries[i].value;
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
            t += 1;
        }

        return buf[0..len];
    }

    pub fn encode(): []const u8 {
        return _.toString();
    }
}

pub const toml = TomlDoc;
pub const TOML = TomlDoc;
