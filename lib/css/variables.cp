// Source: Absorbed from /data/dev/bun/src/css/properties/custom.cp | Synced: 2026-11-22T10:15:30Z
// lib/css/variables.cp
// CSS Custom Properties Registry & var(--name, fallback) Recursive Resolver
// Absorbed from Bun src/css/properties/custom.cp

const std = @import("std");

pub struct CssVarEntry {
    name: []const u8 = "";
    value: []const u8 = "";
}

pub struct CssVariableMap {
    entries: [64]CssVarEntry = undefined;
    count: usize = 0;

    pub static fn init(): CssVariableMap {
        return CssVariableMap {
            entries: std.mem.zeroes([64]CssVarEntry),
            count: 0,
        };
    }

    pub fn len(): usize {
        return _.count;
    }

    pub fn clear(*_) {
        _.count = 0;
    }

    pub fn set(*_, name: []const u8, value: []const u8): bool {
        const clean_name = std.mem.trim(u8, name, " \t\r\n");
        if (!std.mem.startsWith(u8, clean_name, "--")) {
            return false;
        }
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].name, clean_name)) {
                _.entries[i].value = std.mem.trim(u8, value, " \t\r\n");
                return true;
            }
            i += 1;
        }
        if (_.count >= 64) {
            return false;
        }
        _.entries[_.count] = CssVarEntry {
            name: clean_name,
            value: std.mem.trim(u8, value, " \t\r\n"),
        };
        _.count += 1;
        return true;
    }

    pub fn get(name: []const u8): ([]const u8)? {
        const clean_name = std.mem.trim(u8, name, " \t\r\n");
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].name, clean_name)) {
                return _.entries[i].value;
            }
            i += 1;
        }
        return null;
    }

    pub fn has(name: []const u8): bool {
        return _.get(name) != null;
    }

    pub fn remove(*_, name: []const u8): bool {
        const clean_name = std.mem.trim(u8, name, " \t\r\n");
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].name, clean_name)) {
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
}

fn appendBytes(out_buf: []u8, pos: usize, s: []const u8): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out_buf.len) {
        out_buf[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn findMatchingParen(s: []const u8, open_idx: usize): usize? {
    let depth: usize = 1;
    let i = open_idx + 1;
    let in_quote: u8 = 0;
    while (i < s.len) {
        const c = s[i];
        if (in_quote != 0) {
            if (c == '\\' && i + 1 < s.len) {
                i += 2;
                continue;
            }
            if (c == in_quote) {
                in_quote = 0;
            }
        } else {
            if (c == '"' || c == '\'') {
                in_quote = c;
            } else if (c == '(') {
                depth += 1;
            } else if (c == ')') {
                depth -= 1;
                if (depth == 0) {
                    return i;
                }
            }
        }
        i += 1;
    }
    return null;
}

fn findTopLevelComma(s: []const u8): usize? {
    let depth: usize = 0;
    let i: usize = 0;
    let in_quote: u8 = 0;
    while (i < s.len) {
        const c = s[i];
        if (in_quote != 0) {
            if (c == '\\' && i + 1 < s.len) {
                i += 2;
                continue;
            }
            if (c == in_quote) {
                in_quote = 0;
            }
        } else {
            if (c == '"' || c == '\'') {
                in_quote = c;
            } else if (c == '(') {
                depth += 1;
            } else if (c == ')') {
                if (depth > 0) depth -= 1;
            } else if (c == ',' && depth == 0) {
                return i;
            }
        }
        i += 1;
    }
    return null;
}

fn resolveInternal(value: []const u8, vars: CssVariableMap, out_buf: []u8, depth: usize): []const u8 {
    if (depth >= 16) {
        const n = appendBytes(out_buf, 0, value);
        return out_buf[0..n];
    }

    let pos: usize = 0;
    let i: usize = 0;
    while (i < value.len) {
        if (i + 4 <= value.len && std.ascii.eqlIgnoreCase(value[i .. i + 4], "var(")) {
            if (close_idx := findMatchingParen(value, i + 3)) {
                const inner = value[i + 4 .. close_idx];
                let var_name = inner;
                let fallback: ([]const u8)? = null;
                if (comma_idx := findTopLevelComma(inner)) {
                    var_name = inner[0..comma_idx];
                    fallback = std.mem.trim(u8, inner[comma_idx + 1 ..], " \t\r\n");
                }
                var_name = std.mem.trim(u8, var_name, " \t\r\n");

                let sub_buf: [512]u8 = [0; 512];
                if (resolved_val := vars.get(var_name)) {
                    const expanded = resolveInternal(resolved_val, vars, sub_buf[0..512], depth + 1);
                    pos = appendBytes(out_buf, pos, expanded);
                } else if (fb := fallback) {
                    const expanded = resolveInternal(fb, vars, sub_buf[0..512], depth + 1);
                    pos = appendBytes(out_buf, pos, expanded);
                } else {
                    pos = appendBytes(out_buf, pos, value[i .. close_idx + 1]);
                }
                i = close_idx + 1;
                continue;
            }
        }
        if (pos < out_buf.len) {
            out_buf[pos] = value[i];
            pos += 1;
        }
        i += 1;
    }
    return out_buf[0..pos];
}

pub struct CssVarResolver {
    /// Resolves `var(--name)` and `var(--name, fallback)` occurrences recursively (max depth 16).
    pub static fn resolve(value: []const u8, vars: CssVariableMap, out_buf: []u8): []const u8 {
        return resolveInternal(value, vars, out_buf, 0);
    }
}
