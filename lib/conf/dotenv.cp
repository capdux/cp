// lib/conf/dotenv.cp
// High-performance, zero-allocation .env loader in CP

const std = @import("std");

pub struct Entry {
    key: []const u8;
    value: []const u8;
}

pub struct DotEnv {
    entries: [128]Entry = undefined;
    count: usize = 0;
    buf: [4096]u8 = undefined;
    buf_len: usize = 0;

    pub static fn parse(content: []const u8): DotEnv {
        let env = DotEnv {
            entries: undefined,
            count: 0,
            buf: undefined,
            buf_len: 0,
        };
        env.parseContent(content);
        return env;
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

    fn parseContent(*_, content: []const u8) {
        let line_iter = std.mem.splitScalar(u8, content, '\n');
        while (raw_line := line_iter.next()) {
            let line = std.mem.trim(u8, raw_line, " \t\r");
            if (line.len == 0 || line[0] == '#') {
                continue;
            }

            // Strip "export " prefix if present
            if (std.mem.startsWith(u8, line, "export ") || std.mem.startsWith(u8, line, "export\t")) {
                line = std.mem.trim(u8, line[6..], " \t");
            }

            // Find delimiter '=' or ':'
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
            _.put(raw_key, _.parseValue(raw_val));
        }
    }

    fn parseValue(*_, raw: []const u8): []const u8 {
        if (raw.len == 0) {
            return "";
        }

        // Double quoted: expand \n, \r, \t, etc.
        if (raw[0] == '"') {
            const end_idx = std.mem.lastIndexOfScalar(u8, raw, '"');
            if (end_idx != null && end_idx.? > 0) {
                const inner = raw[1..end_idx.?];
                return _.unescapeDoubleQuoted(inner);
            }
        }

        // Single quoted: raw literal
        if (raw[0] == '\'') {
            const end_idx = std.mem.lastIndexOfScalar(u8, raw, '\'');
            if (end_idx != null && end_idx.? > 0) {
                return raw[1..end_idx.?];
            }
        }

        // Unquoted: strip trailing inline comments
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

    fn unescapeDoubleQuoted(*_, inner: []const u8): []const u8 {
        const start_buf_pos = _.buf_len;
        let i: usize = 0;
        while (i < inner.len) {
            if (inner[i] == '\\' && i + 1 < inner.len) {
                const next_char = inner[i + 1];
                if (next_char == 'n') {
                    _.appendChar('\n');
                } else if (next_char == 'r') {
                    _.appendChar('\r');
                } else if (next_char == 't') {
                    _.appendChar('\t');
                } else if (next_char == '\\') {
                    _.appendChar('\\');
                } else if (next_char == '"') {
                    _.appendChar('"');
                } else {
                    _.appendChar('\\');
                    _.appendChar(next_char);
                }
                i += 2;
            } else {
                _.appendChar(inner[i]);
                i += 1;
            }
        }
        return std.heap.page_allocator.dupe(u8, _.buf[start_buf_pos.._.buf_len]) catch "";
    }

    fn appendChar(*_, c: u8) {
        if (_.buf_len < _.buf.len) {
            _.buf[_.buf_len] = c;
            _.buf_len += 1;
        }
    }

    fn put(*_, key: []const u8, value: []const u8) {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].key, key)) {
                _.entries[i].value = value;
                return;
            }
            i += 1;
        }

        if (_.count < _.entries.len) {
            _.entries[_.count] = Entry {
                key: key,
                value: value,
            };
            _.count += 1;
        }
    }
}

pub const dotenv = struct {
    pub const parse = DotEnv.parse;
};
