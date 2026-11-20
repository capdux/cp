// Source: cp/lib/interchange/bridge.cp (TOML & YAML Bridge + Lossless Serializer)
// Bridges pure CP cap:toml and cap:yaml parsers to InterchangeValue and provides TOML/YAML serializers.

const std = @import("std");
import { TomlParser, TomlDoc, TomlValue, TomlError } from "cap:toml";
import { YamlParser, YamlDoc, YamlValue, YamlError } from "cap:yaml";
import {
    INTERCHANGE_MAX_DEPTH,
    InterchangeError,
    InterchangeEntry,
    InterchangeValue,
} from "./value.cp";

pub class TomlYamlBridge {
    static fn mapTomlError(err_val: anyerror): InterchangeError {
        return switch (err_val) {
            TomlError.UnexpectedEof => InterchangeError.UnexpectedEof,
            TomlError.UnexpectedChar => InterchangeError.UnexpectedChar,
            TomlError.UnterminatedString => InterchangeError.UnterminatedString,
            TomlError.InvalidEscape => InterchangeError.InvalidEscape,
            TomlError.DuplicateKey => InterchangeError.DuplicateKey,
            TomlError.TypeConflict => InterchangeError.TypeConflict,
            TomlError.InvalidNumber | TomlError.InvalidDateTime => InterchangeError.InvalidNumber,
            TomlError.MaxDepthExceeded => InterchangeError.MaxDepthExceeded,
            TomlError.OutOfMemory => InterchangeError.OutOfMemory,
            else => InterchangeError.UnexpectedToken,
        };
    }

    static fn mapYamlError(err_val: anyerror): InterchangeError {
        return switch (err_val) {
            YamlError.UnexpectedEof => InterchangeError.UnexpectedEof,
            YamlError.InvalidIndentation => InterchangeError.InvalidIndentation,
            YamlError.TabIndentationForbidden => InterchangeError.TabIndentationForbidden,
            YamlError.DuplicateKey => InterchangeError.DuplicateKey,
            YamlError.UnterminatedString | YamlError.UnterminatedFlow => InterchangeError.UnterminatedString,
            YamlError.MaxDepthExceeded => InterchangeError.MaxDepthExceeded,
            YamlError.OutOfMemory => InterchangeError.OutOfMemory,
            else => InterchangeError.UnexpectedToken,
        };
    }

    pub static fn parseToml(alloc: std.mem.Allocator, input: []const u8): InterchangeValue! {
        let doc = TomlParser.parse(alloc, input) catch e => {
            return TomlYamlBridge.mapTomlError(e);
        };
        defer doc.deinit();
        return TomlYamlBridge.fromTomlTable(alloc, doc.root, 0);
    }

    static fn fromTomlTable(alloc: std.mem.Allocator, tbl: anytype, depth: usize): InterchangeValue! {
        if (depth > INTERCHANGE_MAX_DEPTH) return InterchangeError.MaxDepthExceeded;
        const src_entries = tbl.entries.items;
        if (src_entries.len == 0) return InterchangeValue.fromObject([]);
        let out = alloc.alloc(InterchangeEntry, src_entries.len) catch return InterchangeError.OutOfMemory;
        let filled: usize = 0;
        errdefer {
            let j: usize = 0;
            while (j < filled) : (j += 1) {
                if (out[j].key.len > 0) alloc.free(out[j].key);
                out[j].value.deinit(alloc);
            }
            alloc.free(out);
        }
        while (filled < src_entries.len) : (filled += 1) {
            const e = src_entries[filled];
            const k: []const u8 = if (e.key.len > 0)
                (alloc.dupe(u8, e.key) catch return InterchangeError.OutOfMemory)
            else
                "";
            errdefer {
                if (k.len > 0) alloc.free(k);
            }
            const v = TomlYamlBridge.fromTomlValue(alloc, e.value, depth + 1)?;
            out[filled] = InterchangeEntry{ key: k, value: v };
        }
        return InterchangeValue.fromObject(out);
    }

    static fn fromTomlValue(alloc: std.mem.Allocator, tv: TomlValue, depth: usize): InterchangeValue! {
        if (depth > INTERCHANGE_MAX_DEPTH) return InterchangeError.MaxDepthExceeded;
        switch (tv.kind) {
            .String | .DateTime => return InterchangeValue.fromString(alloc, tv.str_val),
            .Integer => return InterchangeValue.fromInt(tv.int_val),
            .Float => return InterchangeValue.fromFloat(tv.float_val),
            .Boolean => return InterchangeValue.fromBool(tv.bool_val),
            .Array => {
                const src_items = tv.arr_val.items.items;
                if (src_items.len == 0) return InterchangeValue.fromArray([]);
                let out = alloc.alloc(InterchangeValue, src_items.len) catch return InterchangeError.OutOfMemory;
                let filled: usize = 0;
                errdefer {
                    let j: usize = 0;
                    while (j < filled) : (j += 1) {
                        out[j].deinit(alloc);
                    }
                    alloc.free(out);
                }
                while (filled < src_items.len) : (filled += 1) {
                    out[filled] = TomlYamlBridge.fromTomlValue(alloc, src_items[filled], depth + 1)?;
                }
                return InterchangeValue.fromArray(out);
            },
            .Table => return TomlYamlBridge.fromTomlTable(alloc, tv.table_val, depth),
        }
    }

    pub static fn parseYaml(alloc: std.mem.Allocator, input: []const u8): InterchangeValue! {
        let doc = YamlParser.parse(alloc, input) catch e => {
            return TomlYamlBridge.mapYamlError(e);
        };
        defer doc.deinit();
        return TomlYamlBridge.fromYamlValue(alloc, doc.root, 0);
    }

    static fn fromYamlValue(alloc: std.mem.Allocator, yv: YamlValue, depth: usize): InterchangeValue! {
        if (depth > INTERCHANGE_MAX_DEPTH) return InterchangeError.MaxDepthExceeded;
        switch (yv.kind) {
            .Null => return InterchangeValue.initNull(),
            .Bool => return InterchangeValue.fromBool(yv.bool_val),
            .Int => return InterchangeValue.fromInt(yv.int_val),
            .Float => return InterchangeValue.fromFloat(yv.float_val),
            .String => return InterchangeValue.fromString(alloc, yv.str_val),
            .Sequence => {
                if (yv.seq_val.len == 0) return InterchangeValue.fromArray([]);
                let out = alloc.alloc(InterchangeValue, yv.seq_val.len) catch return InterchangeError.OutOfMemory;
                let filled: usize = 0;
                errdefer {
                    let j: usize = 0;
                    while (j < filled) : (j += 1) {
                        out[j].deinit(alloc);
                    }
                    alloc.free(out);
                }
                while (filled < yv.seq_val.len) : (filled += 1) {
                    out[filled] = TomlYamlBridge.fromYamlValue(alloc, yv.seq_val[filled], depth + 1)?;
                }
                return InterchangeValue.fromArray(out);
            },
            .Mapping => {
                if (yv.map_val.len == 0) return InterchangeValue.fromObject([]);
                let out = alloc.alloc(InterchangeEntry, yv.map_val.len) catch return InterchangeError.OutOfMemory;
                let filled: usize = 0;
                errdefer {
                    let j: usize = 0;
                    while (j < filled) : (j += 1) {
                        if (out[j].key.len > 0) alloc.free(out[j].key);
                        out[j].value.deinit(alloc);
                    }
                    alloc.free(out);
                }
                while (filled < yv.map_val.len) : (filled += 1) {
                    const e = yv.map_val[filled];
                    const k: []const u8 = if (e.key.len > 0)
                        (alloc.dupe(u8, e.key) catch return InterchangeError.OutOfMemory)
                    else
                        "";
                    errdefer {
                        if (k.len > 0) alloc.free(k);
                    }
                    const v = TomlYamlBridge.fromYamlValue(alloc, e.value, depth + 1)?;
                    out[filled] = InterchangeEntry{ key: k, value: v };
                }
                return InterchangeValue.fromObject(out);
            },
        }
    }

    pub static fn stringifyToml(alloc: std.mem.Allocator, val: InterchangeValue): ([]u8)! {
        if (val.kind != .Object) return InterchangeError.TypeConflict;
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        TomlYamlBridge.writeTomlTable(alloc, &out, val.obj_val, "", 0)?;
        return out.toOwnedSlice(alloc) catch return InterchangeError.OutOfMemory;
    }

    static fn isBareTomlKey(k: []const u8): bool {
        if (k.len == 0) return false;
        for (c of k) {
            const ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
            if (!ok) return false;
        }
        return true;
    }

    static fn writeTomlKey(alloc: std.mem.Allocator, out: *std.ArrayList(u8), key: []const u8): ! {
        if (TomlYamlBridge.isBareTomlKey(key)) {
            out.appendSlice(alloc, key) catch return InterchangeError.OutOfMemory;
        } else {
            TomlYamlBridge.writeTomlString(alloc, out, key)?;
        }
    }

    static fn writeTomlString(alloc: std.mem.Allocator, out: *std.ArrayList(u8), s: []const u8): ! {
        out.append(alloc, '"') catch return InterchangeError.OutOfMemory;
        for (c of s) {
            switch (c) {
                '"' => out.appendSlice(alloc, "\\\"") catch return InterchangeError.OutOfMemory,
                '\\' => out.appendSlice(alloc, "\\\\") catch return InterchangeError.OutOfMemory,
                '\n' => out.appendSlice(alloc, "\\n") catch return InterchangeError.OutOfMemory,
                '\r' => out.appendSlice(alloc, "\\r") catch return InterchangeError.OutOfMemory,
                '\t' => out.appendSlice(alloc, "\\t") catch return InterchangeError.OutOfMemory,
                0x08 => out.appendSlice(alloc, "\\b") catch return InterchangeError.OutOfMemory,
                0x0C => out.appendSlice(alloc, "\\f") catch return InterchangeError.OutOfMemory,
                else => {
                    if (c < 0x20) {
                        const hex = "0123456789abcdef";
                        out.appendSlice(alloc, "\\u00") catch return InterchangeError.OutOfMemory;
                        out.append(alloc, hex[(c >> 4) & 0x0F]) catch return InterchangeError.OutOfMemory;
                        out.append(alloc, hex[c & 0x0F]) catch return InterchangeError.OutOfMemory;
                    } else {
                        out.append(alloc, c) catch return InterchangeError.OutOfMemory;
                    }
                },
            }
        }
        out.append(alloc, '"') catch return InterchangeError.OutOfMemory;
    }

    static fn isArrayOfObjects(items: []InterchangeValue): bool {
        if (items.len == 0) return false;
        for (it of items) {
            if (it.kind != .Object) return false;
        }
        return true;
    }

    static fn writeTomlInlineValue(alloc: std.mem.Allocator, out: *std.ArrayList(u8), val: InterchangeValue, depth: usize): ! {
        if (depth > INTERCHANGE_MAX_DEPTH) return InterchangeError.MaxDepthExceeded;
        switch (val.kind) {
            .Null => out.appendSlice(alloc, "\"\"") catch return InterchangeError.OutOfMemory,
            .Bool => out.appendSlice(alloc, if (val.bool_val) "true" else "false") catch return InterchangeError.OutOfMemory,
            .Int => {
                let buf: [32]u8 = undefined;
                const s = std.fmt.bufPrint(&buf, "{d}", .{val.int_val}) catch return InterchangeError.OutOfMemory;
                out.appendSlice(alloc, s) catch return InterchangeError.OutOfMemory;
            },
            .Float => {
                if (std.math.isNan(val.float_val)) {
                    out.appendSlice(alloc, "nan") catch return InterchangeError.OutOfMemory;
                } else if (std.math.isInf(val.float_val)) {
                    out.appendSlice(alloc, if (val.float_val < 0) "-inf" else "inf") catch return InterchangeError.OutOfMemory;
                } else {
                    let buf: [64]u8 = undefined;
                    const s = std.fmt.bufPrint(&buf, "{d}", .{val.float_val}) catch return InterchangeError.OutOfMemory;
                    out.appendSlice(alloc, s) catch return InterchangeError.OutOfMemory;
                    if (std.mem.indexOfScalar(u8, s, '.') == null && std.mem.indexOfScalar(u8, s, 'e') == null && std.mem.indexOfScalar(u8, s, 'E') == null) {
                        out.appendSlice(alloc, ".0") catch return InterchangeError.OutOfMemory;
                    }
                }
            },
            .String => TomlYamlBridge.writeTomlString(alloc, out, val.str_val)?,
            .Array => {
                out.append(alloc, '[') catch return InterchangeError.OutOfMemory;
                let i: usize = 0;
                while (i < val.arr_val.len) : (i += 1) {
                    if (i > 0) out.appendSlice(alloc, ", ") catch return InterchangeError.OutOfMemory;
                    TomlYamlBridge.writeTomlInlineValue(alloc, out, val.arr_val[i], depth + 1)?;
                }
                out.append(alloc, ']') catch return InterchangeError.OutOfMemory;
            },
            .Object => {
                out.appendSlice(alloc, "{ ") catch return InterchangeError.OutOfMemory;
                let i: usize = 0;
                while (i < val.obj_val.len) : (i += 1) {
                    if (i > 0) out.appendSlice(alloc, ", ") catch return InterchangeError.OutOfMemory;
                    TomlYamlBridge.writeTomlKey(alloc, out, val.obj_val[i].key)?;
                    out.appendSlice(alloc, " = ") catch return InterchangeError.OutOfMemory;
                    TomlYamlBridge.writeTomlInlineValue(alloc, out, val.obj_val[i].value, depth + 1)?;
                }
                out.appendSlice(alloc, " }") catch return InterchangeError.OutOfMemory;
            },
        }
    }

    static fn buildTomlPath(alloc: std.mem.Allocator, prefix: []const u8, key: []const u8): ([]u8)! {
        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(alloc);
        if (prefix.len > 0) {
            buf.appendSlice(alloc, prefix) catch return InterchangeError.OutOfMemory;
            buf.append(alloc, '.') catch return InterchangeError.OutOfMemory;
        }
        TomlYamlBridge.writeTomlKey(alloc, &buf, key)?;
        return buf.toOwnedSlice(alloc) catch return InterchangeError.OutOfMemory;
    }

    static fn writeTomlTable(
        alloc: std.mem.Allocator,
        out: *std.ArrayList(u8),
        entries: []InterchangeEntry,
        prefix: []const u8,
        depth: usize,
    ): ! {
        if (depth > INTERCHANGE_MAX_DEPTH) return InterchangeError.MaxDepthExceeded;

        for (entry of entries) {
            if (entry.value.kind == .Object) continue;
            if (entry.value.kind == .Array && TomlYamlBridge.isArrayOfObjects(entry.value.arr_val)) continue;
            TomlYamlBridge.writeTomlKey(alloc, out, entry.key)?;
            out.appendSlice(alloc, " = ") catch return InterchangeError.OutOfMemory;
            TomlYamlBridge.writeTomlInlineValue(alloc, out, entry.value, depth + 1)?;
            out.append(alloc, '\n') catch return InterchangeError.OutOfMemory;
        }

        for (entry of entries) {
            if (entry.value.kind == .Object) {
                const sub_path = TomlYamlBridge.buildTomlPath(alloc, prefix, entry.key)?;
                defer alloc.free(sub_path);
                if (out.items.len > 0) out.append(alloc, '\n') catch return InterchangeError.OutOfMemory;
                out.append(alloc, '[') catch return InterchangeError.OutOfMemory;
                out.appendSlice(alloc, sub_path) catch return InterchangeError.OutOfMemory;
                out.appendSlice(alloc, "]\n") catch return InterchangeError.OutOfMemory;
                TomlYamlBridge.writeTomlTable(alloc, out, entry.value.obj_val, sub_path, depth + 1)?;
            } else if (entry.value.kind == .Array && TomlYamlBridge.isArrayOfObjects(entry.value.arr_val)) {
                const sub_path = TomlYamlBridge.buildTomlPath(alloc, prefix, entry.key)?;
                defer alloc.free(sub_path);
                for (item of entry.value.arr_val) {
                    if (out.items.len > 0) out.append(alloc, '\n') catch return InterchangeError.OutOfMemory;
                    out.appendSlice(alloc, "[[") catch return InterchangeError.OutOfMemory;
                    out.appendSlice(alloc, sub_path) catch return InterchangeError.OutOfMemory;
                    out.appendSlice(alloc, "]]\n") catch return InterchangeError.OutOfMemory;
                    TomlYamlBridge.writeTomlTable(alloc, out, item.obj_val, sub_path, depth + 1)?;
                }
            }
        }
    }

    pub static fn stringifyYaml(alloc: std.mem.Allocator, val: InterchangeValue): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        TomlYamlBridge.writeYamlValue(alloc, &out, val, 0, false)?;
        if (out.items.len == 0 || out.items[out.items.len - 1] != '\n') {
            out.append(alloc, '\n') catch return InterchangeError.OutOfMemory;
        }
        return out.toOwnedSlice(alloc) catch return InterchangeError.OutOfMemory;
    }

    static fn writeIndent(alloc: std.mem.Allocator, out: *std.ArrayList(u8), level: usize): ! {
        let i: usize = 0;
        while (i < level) : (i += 1) {
            out.appendSlice(alloc, "  ") catch return InterchangeError.OutOfMemory;
        }
    }

    static fn needsYamlQuotes(s: []const u8): bool {
        if (s.len == 0) return true;
        if (s[0] == ' ' || s[s.len - 1] == ' ') return true;
        if (s[0] == '-' || s[0] == '?' || s[0] == ':' || s[0] == '@' || s[0] == '`' || s[0] == '&' || s[0] == '*' || s[0] == '!' || s[0] == '|' || s[0] == '>' || s[0] == '\'' || s[0] == '"' || s[0] == '%' || s[0] == '{' || s[0] == '}' || s[0] == '[' || s[0] == ']' || s[0] == '#') {
            return true;
        }
        if (std.mem.eql(u8, s, "true") || std.mem.eql(u8, s, "false") || std.mem.eql(u8, s, "null") || std.mem.eql(u8, s, "~")) {
            return true;
        }
        for (c of s) {
            if (c < 0x20 || c == ':' || c == '#' || c == '"' || c == '\'') return true;
        }
        if (iv := (std.fmt.parseInt(i64, s, 10) catch null)) {
            * = iv;
            return true;
        }
        if (fv := (std.fmt.parseFloat(f64, s) catch null)) {
            * = fv;
            return true;
        }
        return false;
    }

    static fn writeYamlString(alloc: std.mem.Allocator, out: *std.ArrayList(u8), s: []const u8): ! {
        if (!TomlYamlBridge.needsYamlQuotes(s)) {
            out.appendSlice(alloc, s) catch return InterchangeError.OutOfMemory;
            return;
        }
        TomlYamlBridge.writeTomlString(alloc, out, s)?;
    }

    static fn writeYamlValue(
        alloc: std.mem.Allocator,
        out: *std.ArrayList(u8),
        val: InterchangeValue,
        indent: usize,
        inline_prefix: bool,
    ): ! {
        if (indent > INTERCHANGE_MAX_DEPTH) return InterchangeError.MaxDepthExceeded;
        switch (val.kind) {
            .Null => out.appendSlice(alloc, "null") catch return InterchangeError.OutOfMemory,
            .Bool => out.appendSlice(alloc, if (val.bool_val) "true" else "false") catch return InterchangeError.OutOfMemory,
            .Int => {
                let buf: [32]u8 = undefined;
                const s = std.fmt.bufPrint(&buf, "{d}", .{val.int_val}) catch return InterchangeError.OutOfMemory;
                out.appendSlice(alloc, s) catch return InterchangeError.OutOfMemory;
            },
            .Float => {
                if (std.math.isNan(val.float_val)) {
                    out.appendSlice(alloc, ".nan") catch return InterchangeError.OutOfMemory;
                } else if (std.math.isInf(val.float_val)) {
                    out.appendSlice(alloc, if (val.float_val < 0) "-.inf" else ".inf") catch return InterchangeError.OutOfMemory;
                } else {
                    let buf: [64]u8 = undefined;
                    const s = std.fmt.bufPrint(&buf, "{d}", .{val.float_val}) catch return InterchangeError.OutOfMemory;
                    out.appendSlice(alloc, s) catch return InterchangeError.OutOfMemory;
                }
            },
            .String => TomlYamlBridge.writeYamlString(alloc, out, val.str_val)?,
            .Array => {
                if (val.arr_val.len == 0) {
                    out.appendSlice(alloc, "[]") catch return InterchangeError.OutOfMemory;
                    return;
                }
                let i: usize = 0;
                while (i < val.arr_val.len) : (i += 1) {
                    if (i > 0 || inline_prefix) {
                        out.append(alloc, '\n') catch return InterchangeError.OutOfMemory;
                        TomlYamlBridge.writeIndent(alloc, out, indent)?;
                    }
                    out.appendSlice(alloc, "- ") catch return InterchangeError.OutOfMemory;
                    const item = val.arr_val[i];
                    if (item.kind == .Object && item.obj_val.len > 0) {
                        let k: usize = 0;
                        while (k < item.obj_val.len) : (k += 1) {
                            if (k > 0) {
                                out.append(alloc, '\n') catch return InterchangeError.OutOfMemory;
                                TomlYamlBridge.writeIndent(alloc, out, indent + 1)?;
                            }
                            const e = item.obj_val[k];
                            TomlYamlBridge.writeYamlString(alloc, out, e.key)?;
                            out.append(alloc, ':') catch return InterchangeError.OutOfMemory;
                            if ((e.value.kind == .Object && e.value.obj_val.len > 0) || (e.value.kind == .Array && e.value.arr_val.len > 0)) {
                                TomlYamlBridge.writeYamlValue(alloc, out, e.value, indent + 2, true)?;
                            } else {
                                out.append(alloc, ' ') catch return InterchangeError.OutOfMemory;
                                TomlYamlBridge.writeYamlValue(alloc, out, e.value, indent + 2, false)?;
                            }
                        }
                    } else if (item.kind == .Array && item.arr_val.len > 0) {
                        TomlYamlBridge.writeYamlValue(alloc, out, item, indent + 1, true)?;
                    } else {
                        TomlYamlBridge.writeYamlValue(alloc, out, item, indent + 1, false)?;
                    }
                }
            },
            .Object => {
                if (val.obj_val.len == 0) {
                    out.appendSlice(alloc, "{}") catch return InterchangeError.OutOfMemory;
                    return;
                }
                let i: usize = 0;
                while (i < val.obj_val.len) : (i += 1) {
                    if (i > 0 || inline_prefix) {
                        out.append(alloc, '\n') catch return InterchangeError.OutOfMemory;
                        TomlYamlBridge.writeIndent(alloc, out, indent)?;
                    }
                    const e = val.obj_val[i];
                    TomlYamlBridge.writeYamlString(alloc, out, e.key)?;
                    out.append(alloc, ':') catch return InterchangeError.OutOfMemory;
                    if ((e.value.kind == .Object && e.value.obj_val.len > 0) || (e.value.kind == .Array && e.value.arr_val.len > 0)) {
                        TomlYamlBridge.writeYamlValue(alloc, out, e.value, indent + 1, true)?;
                    } else {
                        out.append(alloc, ' ') catch return InterchangeError.OutOfMemory;
                        TomlYamlBridge.writeYamlValue(alloc, out, e.value, indent + 1, false)?;
                    }
                }
            },
        }
    }
}
