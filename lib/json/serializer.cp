// Source: /data/dev/bun/src/interchange/json.cp
// lib/json/serializer.cp
// RFC 8259 Compliant Compact & Pretty JSON Serializer for JsonValue

const std = @import("std");
import { JsonValue } from "./value";

pub struct JsonSerializer {
    static fn appendEscapedString(out: *std.ArrayList(u8), allocator: std.mem.Allocator, s: []const u8): ! {
        out.append(allocator, '"')?;
        let i: usize = 0;
        let flush_start: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            if (c == '"' || c == '\\' || c < 0x20) {
                if (i > flush_start) {
                    out.appendSlice(allocator, s[flush_start..i])?;
                }
                switch (c) {
                    '"' => out.appendSlice(allocator, "\\\"")?,
                    '\\' => out.appendSlice(allocator, "\\\\")?,
                    0x08 => out.appendSlice(allocator, "\\b")?,
                    0x0C => out.appendSlice(allocator, "\\f")?,
                    '\n' => out.appendSlice(allocator, "\\n")?,
                    '\r' => out.appendSlice(allocator, "\\r")?,
                    '\t' => out.appendSlice(allocator, "\\t")?,
                    else => {
                        const hex_digits = "0123456789abcdef";
                        let seq: [6]u8 = ['\\', 'u', '0', '0', hex_digits[(c >> 4) & 0xF], hex_digits[c & 0xF]];
                        out.appendSlice(allocator, seq[0..6])?;
                    },
                }
                flush_start = i + 1;
            }
        }
        if (i > flush_start) {
            out.appendSlice(allocator, s[flush_start..i])?;
        }
        out.append(allocator, '"')?;
    }

    static fn appendNumber(out: *std.ArrayList(u8), allocator: std.mem.Allocator, v: f64): ! {
        zig {
            if (!std.math.isFinite(v)) {
                try out.appendSlice(allocator, "null");
                return;
            }
            if (v == 0.0) {
                if (std.math.signbit(v)) {
                    try out.appendSlice(allocator, "-0");
                } else {
                    try out.appendSlice(allocator, "0");
                }
                return;
            }
            if (@abs(v) < 9007199254740992.0 and @trunc(v) == v) {
                const int_val: i64 = @intFromFloat(v);
                var buf: [32]u8 = undefined;
                const s = std.fmt.bufPrint(&buf, "{d}", .{int_val}) catch unreachable;
                try out.appendSlice(allocator, s);
                return;
            }
            var buf: [64]u8 = undefined;
            const s = std.fmt.bufPrint(&buf, "{d}", .{v}) catch unreachable;
            try out.appendSlice(allocator, s);
        }
    }

    static fn appendIndent(out: *std.ArrayList(u8), allocator: std.mem.Allocator, spaces: usize): ! {
        let i: usize = 0;
        while (i < spaces) : (i += 1) {
            out.append(allocator, ' ')?;
        }
    }

    static fn writeValue(out: *std.ArrayList(u8), allocator: std.mem.Allocator, val: JsonValue, indent: usize, depth: usize): ! {
        switch (val.tag) {
            .Null => out.appendSlice(allocator, "null")?,
            .Bool => {
                if (val.bool_val) {
                    out.appendSlice(allocator, "true")?;
                } else {
                    out.appendSlice(allocator, "false")?;
                }
            },
            .Number => JsonSerializer.appendNumber(out, allocator, val.num_val)?,
            .String => JsonSerializer.appendEscapedString(out, allocator, val.str_val)?,
            .Array => {
                if (val.arr_val.len == 0) {
                    out.appendSlice(allocator, "[]")?;
                    return;
                }
                out.append(allocator, '[')?;
                if (indent > 0) out.append(allocator, '\n')?;
                let i: usize = 0;
                while (i < val.arr_val.len) : (i += 1) {
                    if (i > 0) {
                        out.append(allocator, ',')?;
                        if (indent > 0) out.append(allocator, '\n')?;
                    }
                    if (indent > 0) {
                        JsonSerializer.appendIndent(out, allocator, (depth + 1) * indent)?;
                    }
                    JsonSerializer.writeValue(out, allocator, val.arr_val[i], indent, depth + 1)?;
                }
                if (indent > 0) {
                    out.append(allocator, '\n')?;
                    JsonSerializer.appendIndent(out, allocator, depth * indent)?;
                }
                out.append(allocator, ']')?;
            },
            .Object => {
                if (val.obj_val.len == 0) {
                    out.appendSlice(allocator, "{}")?;
                    return;
                }
                out.append(allocator, '{')?;
                if (indent > 0) out.append(allocator, '\n')?;
                let i: usize = 0;
                while (i < val.obj_val.len) : (i += 1) {
                    if (i > 0) {
                        out.append(allocator, ',')?;
                        if (indent > 0) out.append(allocator, '\n')?;
                    }
                    if (indent > 0) {
                        JsonSerializer.appendIndent(out, allocator, (depth + 1) * indent)?;
                    }
                    const f = val.obj_val[i];
                    JsonSerializer.appendEscapedString(out, allocator, f.key)?;
                    if (indent > 0) {
                        out.appendSlice(allocator, ": ")?;
                    } else {
                        out.append(allocator, ':')?;
                    }
                    JsonSerializer.writeValue(out, allocator, f.value, indent, depth + 1)?;
                }
                if (indent > 0) {
                    out.append(allocator, '\n')?;
                    JsonSerializer.appendIndent(out, allocator, depth * indent)?;
                }
                out.append(allocator, '}')?;
            },
        }
    }

    /// Serializes a `JsonValue` into compact RFC 8259 JSON text.
    pub static fn stringify(allocator: std.mem.Allocator, val: JsonValue): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);
        JsonSerializer.writeValue(&out, allocator, val, 0, 0)?;
        return out.toOwnedSlice(allocator);
    }

    /// Serializes a `JsonValue` into pretty-printed JSON text using `indent` spaces per level.
    pub static fn stringifyPretty(allocator: std.mem.Allocator, val: JsonValue, indent: usize): ([]u8)! {
        const eff_indent = if (indent == 0) (2 as usize) else indent;
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);
        JsonSerializer.writeValue(&out, allocator, val, eff_indent, 0)?;
        return out.toOwnedSlice(allocator);
    }
}
