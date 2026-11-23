// Source: cp/lib/json/serializer.cp (RFC 8259 Pretty & RFC 8785 Canonical JSON Serializer)
// Reference-hardened against serde-rs/json (MIT/Apache-2.0)
// Pure-CP RFC 8259 Compact/Pretty & RFC 8785 Canonical JSON Serializer for JsonValue
// with 16-lane SIMD unescaped-span bulk copy and zero zig{} blocks.

const std = @import("std");
import { JsonValue, JsonField } from "./value";
import { simdIndexOfJsonEscape } from "./escape.cp";

pub struct JsonSerializeOptions {
    indent_spaces: u8 = 0,
    sort_keys: bool = false,
    escape_html_safe: bool = false,
}

pub struct JsonSerializer {
    static fn appendEscapedByte(
        out: *std.ArrayList(u8),
        allocator: std.mem.Allocator,
        c: u8,
    ): ! {
        switch (c) {
            '"' => out.appendSlice(allocator, "\\\"")?,
            '\\' => out.appendSlice(allocator, "\\\\")?,
            0x08 => out.appendSlice(allocator, "\\b")?,
            0x0C => out.appendSlice(allocator, "\\f")?,
            '\n' => out.appendSlice(allocator, "\\n")?,
            '\r' => out.appendSlice(allocator, "\\r")?,
            '\t' => out.appendSlice(allocator, "\\t")?,
            '<' => out.appendSlice(allocator, "\\u003c")?,
            '>' => out.appendSlice(allocator, "\\u003e")?,
            '&' => out.appendSlice(allocator, "\\u0026")?,
            else => {
                const hex_digits = "0123456789abcdef";
                let seq: [6]u8 = ['\\', 'u', '0', '0', hex_digits[(c >> 4) & 0xF], hex_digits[c & 0xF]];
                out.appendSlice(allocator, seq[0..6])?;
            },
        }
    }

    static fn appendEscapedString(
        out: *std.ArrayList(u8),
        allocator: std.mem.Allocator,
        s: []const u8,
        escape_html_safe: bool,
    ): ! {
        out.append(allocator, '"')?;
        if (!escape_html_safe) {
            // 16-lane SIMD fast path: bulk-copy unescaped spans
            let i: usize = 0;
            while (i < s.len) {
                const rem = s[i..];
                const hit_opt = simdIndexOfJsonEscape(rem);
                if (hit_opt == null) {
                    out.appendSlice(allocator, rem)?;
                    break;
                }
                const hit = hit_opt.?;
                if (hit > 0) {
                    out.appendSlice(allocator, rem[0..hit])?;
                    i += hit;
                }
                JsonSerializer.appendEscapedByte(out, allocator, s[i])?;
                i += 1;
            }
            out.append(allocator, '"')?;
            return;
        }

        let i: usize = 0;
        let flush_start: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            if (c == '"' || c == '\\' || c < 0x20 || c == '<' || c == '>' || c == '&') {
                if (i > flush_start) {
                    out.appendSlice(allocator, s[flush_start..i])?;
                }
                JsonSerializer.appendEscapedByte(out, allocator, c)?;
                flush_start = i + 1;
            }
        }
        if (i > flush_start) {
            out.appendSlice(allocator, s[flush_start..i])?;
        }
        out.append(allocator, '"')?;
    }

    static fn appendNumber(
        out: *std.ArrayList(u8),
        allocator: std.mem.Allocator,
        v: f64,
        canonical_zero: bool,
    ): ! {
        if (!std.math.isFinite(v)) {
            out.appendSlice(allocator, "null")?;
            return;
        }
        if (v == 0.0) {
            if (!canonical_zero && std.math.signbit(v)) {
                out.appendSlice(allocator, "-0")?;
            } else {
                out.appendSlice(allocator, "0")?;
            }
            return;
        }
        if (@abs(v) < 9007199254740992.0 && @trunc(v) == v) {
            const int_val: i64 = @intFromFloat(v);
            let buf: [32]u8 = undefined;
            const s = std.fmt.bufPrint(&buf, "{d}", .{int_val}) catch unreachable;
            out.appendSlice(allocator, s)?;
            return;
        }
        let buf: [64]u8 = undefined;
        const s = std.fmt.bufPrint(&buf, "{d}", .{v}) catch unreachable;
        out.appendSlice(allocator, s)?;
    }

    static fn appendIndent(out: *std.ArrayList(u8), allocator: std.mem.Allocator, spaces: usize): ! {
        let i: usize = 0;
        while (i < spaces) : (i += 1) {
            out.append(allocator, ' ')?;
        }
    }

    /// RFC 8785 §3.2.3 key ordering: lexicographical comparison by UTF-16 code units.
    /// Falls back to byte comparison if input is not valid UTF-8.
    static fn keyLessThan(a: []const u8, b: []const u8): bool {
        let ia: usize = 0;
        let ib: usize = 0;
        while (ia < a.len && ib < b.len) {
            const ba = a[ia];
            const bb = b[ib];
            if (ba < 0x80 && bb < 0x80) {
                if (ba != bb) return ba < bb;
                ia += 1;
                ib += 1;
                continue;
            }
            const la = std.unicode.utf8ByteSequenceLength(ba) catch return std.mem.lessThan(u8, a, b);
            const lb = std.unicode.utf8ByteSequenceLength(bb) catch return std.mem.lessThan(u8, a, b);
            if (ia + la > a.len || ib + lb > b.len) {
                return std.mem.lessThan(u8, a, b);
            }
            const cpa = std.unicode.utf8Decode(a[ia .. ia + la]) catch return std.mem.lessThan(u8, a, b);
            const cpb = std.unicode.utf8Decode(b[ib .. ib + lb]) catch return std.mem.lessThan(u8, a, b);
            if (cpa != cpb) {
                const ua0: u16 = if (cpa <= 0xFFFF) @as(u16, @intCast(cpa)) else @as(u16, @intCast(0xD800 + ((cpa - 0x10000) >> 10)));
                const ub0: u16 = if (cpb <= 0xFFFF) @as(u16, @intCast(cpb)) else @as(u16, @intCast(0xD800 + ((cpb - 0x10000) >> 10)));
                if (ua0 != ub0) return ua0 < ub0;
                const ua1: u16 = if (cpa <= 0xFFFF) (0 as u16) else @as(u16, @intCast(0xDC00 + ((cpa - 0x10000) & 0x3FF)));
                const ub1: u16 = if (cpb <= 0xFFFF) (0 as u16) else @as(u16, @intCast(0xDC00 + ((cpb - 0x10000) & 0x3FF)));
                return ua1 < ub1;
            }
            ia += la;
            ib += lb;
        }
        return a.len < b.len;
    }

    static fn siftDown(fields: []JsonField, start: usize, end: usize) {
        let root = start;
        while (root * 2 + 1 < end) {
            const left = root * 2 + 1;
            const right = left + 1;
            let swap_idx = root;
            if (JsonSerializer.keyLessThan(fields[swap_idx].key, fields[left].key)) {
                swap_idx = left;
            }
            if (right < end && JsonSerializer.keyLessThan(fields[swap_idx].key, fields[right].key)) {
                swap_idx = right;
            }
            if (swap_idx == root) return;
            const tmp = fields[root];
            fields[root] = fields[swap_idx];
            fields[swap_idx] = tmp;
            root = swap_idx;
        }
    }

    /// Pure-CP O(n log n) in-place heapsort by RFC 8785 §3.2.3 UTF-16 key order.
    static fn sortFields(fields: []JsonField) {
        const n = fields.len;
        if (n <= 1) return;
        let i: usize = n / 2;
        while (i > 0) {
            i -= 1;
            JsonSerializer.siftDown(fields, i, n);
        }
        let end: usize = n;
        while (end > 1) {
            end -= 1;
            const tmp = fields[0];
            fields[0] = fields[end];
            fields[end] = tmp;
            JsonSerializer.siftDown(fields, 0, end);
        }
    }

    static fn writeValueOpts(
        out: *std.ArrayList(u8),
        allocator: std.mem.Allocator,
        val: JsonValue,
        indent: usize,
        depth: usize,
        sort_keys: bool,
        escape_html_safe: bool,
        canonical_zero: bool,
    ): ! {
        switch (val.tag) {
            .Null => out.appendSlice(allocator, "null")?,
            .Bool => {
                if (val.bool_val) {
                    out.appendSlice(allocator, "true")?;
                } else {
                    out.appendSlice(allocator, "false")?;
                }
            },
            .Number => JsonSerializer.appendNumber(out, allocator, val.num_val, canonical_zero)?,
            .String => JsonSerializer.appendEscapedString(out, allocator, val.str_val, escape_html_safe)?,
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
                    JsonSerializer.writeValueOpts(
                        out,
                        allocator,
                        val.arr_val[i],
                        indent,
                        depth + 1,
                        sort_keys,
                        escape_html_safe,
                        canonical_zero,
                    )?;
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
                let fields_view: []const JsonField = val.obj_val;
                let sorted_buf: []JsonField = [];
                if (sort_keys && val.obj_val.len > 1) {
                    sorted_buf = allocator.dupe(JsonField, val.obj_val)?;
                    JsonSerializer.sortFields(sorted_buf);
                    fields_view = sorted_buf;
                }
                defer {
                    if (sorted_buf.len > 0) allocator.free(sorted_buf);
                }

                out.append(allocator, '{')?;
                if (indent > 0) out.append(allocator, '\n')?;
                let i: usize = 0;
                while (i < fields_view.len) : (i += 1) {
                    if (i > 0) {
                        out.append(allocator, ',')?;
                        if (indent > 0) out.append(allocator, '\n')?;
                    }
                    if (indent > 0) {
                        JsonSerializer.appendIndent(out, allocator, (depth + 1) * indent)?;
                    }
                    const f = fields_view[i];
                    JsonSerializer.appendEscapedString(out, allocator, f.key, escape_html_safe)?;
                    if (indent > 0) {
                        out.appendSlice(allocator, ": ")?;
                    } else {
                        out.append(allocator, ':')?;
                    }
                    JsonSerializer.writeValueOpts(
                        out,
                        allocator,
                        f.value,
                        indent,
                        depth + 1,
                        sort_keys,
                        escape_html_safe,
                        canonical_zero,
                    )?;
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
        JsonSerializer.writeValueOpts(&out, allocator, val, 0, 0, false, false, false)?;
        return out.toOwnedSlice(allocator);
    }

    /// Serializes a `JsonValue` with full `JsonSerializeOptions` control.
    pub static fn stringifyWithOptions(
        allocator: std.mem.Allocator,
        val: JsonValue,
        opts: JsonSerializeOptions,
    ): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);
        JsonSerializer.writeValueOpts(
            &out,
            allocator,
            val,
            opts.indent_spaces as usize,
            0,
            opts.sort_keys,
            opts.escape_html_safe,
            false,
        )?;
        return out.toOwnedSlice(allocator);
    }

    /// Serializes a `JsonValue` into pretty-printed JSON text using `indent` spaces per level.
    pub static fn stringifyPretty(allocator: std.mem.Allocator, val: JsonValue, indent: usize): ([]u8)! {
        const eff_indent = if (indent == 0) (2 as usize) else indent;
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);
        JsonSerializer.writeValueOpts(&out, allocator, val, eff_indent, 0, false, false, false)?;
        return out.toOwnedSlice(allocator);
    }

    /// Serializes a `JsonValue` into deterministic RFC 8785 Canonical JSON (sorted keys, compact, `-0` -> `0`).
    pub static fn stringifyCanonical(allocator: std.mem.Allocator, val: JsonValue): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);
        JsonSerializer.writeValueOpts(&out, allocator, val, 0, 0, true, false, true)?;
        return out.toOwnedSlice(allocator);
    }
}
