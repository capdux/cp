// Source: cp/lib/toml/codec.cp (TOML v1.0.0 Parser, Conflict Guard & TOML/JSON Serializer)
// Pure CP recursive-descent TOML v1.0.0 parser with strict conflict detection,
// 4 string literal forms, Unicode escapes, RFC 3339 datetimes, max-depth guard (64),
// and TOML / RFC 8259 JSON serializers.

const std = @import("std");
import { TomlError, TomlKind, TomlEntry, TomlArray, TomlTable, TomlValue, TomlDoc } from "./value.cp";

pub const TOML_MAX_DEPTH: usize = 64;

type TomlCodecVec16 = @Vector(16, u8);

fn tomlSimdFindEol(s: []const u8, start: usize): usize {
    let i: usize = start;
    const n = s.len;
    const v_lf: TomlCodecVec16 = @splat('\n');
    const v_cr: TomlCodecVec16 = @splat('\r');
    while (i + 16 <= n) {
        const chunk: TomlCodecVec16 = s[i..][0..16].*;
        const m_lf: u16 = @bitCast(chunk == v_lf);
        const m_cr: u16 = @bitCast(chunk == v_cr);
        const mask: u16 = m_lf | m_cr;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n && s[i] != '\n' && s[i] != '\r') : (i += 1) {}
    return i;
}

fn tomlSimdSkipBareKey(s: []const u8, start: usize): usize {
    let i: usize = start;
    const n = s.len;
    const v_A: TomlCodecVec16 = @splat('A');
    const v_Z: TomlCodecVec16 = @splat('Z');
    const v_a: TomlCodecVec16 = @splat('a');
    const v_z: TomlCodecVec16 = @splat('z');
    const v_0: TomlCodecVec16 = @splat('0');
    const v_9: TomlCodecVec16 = @splat('9');
    const v_us: TomlCodecVec16 = @splat('_');
    const v_hy: TomlCodecVec16 = @splat('-');
    while (i + 16 <= n) {
        const chunk: TomlCodecVec16 = s[i..][0..16].*;
        const m_up: u16 = @as(u16, @bitCast(chunk >= v_A)) & @as(u16, @bitCast(chunk <= v_Z));
        const m_lo: u16 = @as(u16, @bitCast(chunk >= v_a)) & @as(u16, @bitCast(chunk <= v_z));
        const m_dg: u16 = @as(u16, @bitCast(chunk >= v_0)) & @as(u16, @bitCast(chunk <= v_9));
        const m_us: u16 = @bitCast(chunk == v_us);
        const m_hy: u16 = @bitCast(chunk == v_hy);
        const m_bad: u16 = ~(m_up | m_lo | m_dg | m_us | m_hy);
        if (m_bad != 0) {
            return i + (@ctz(m_bad) as usize);
        }
        i += 16;
    }
    while (i < n && TomlParser.isBareKeyChar(s[i])) : (i += 1) {}
    return i;
}

fn tomlSimdFindBasicStrSpecial(s: []const u8, start: usize): usize {
    let i: usize = start;
    const n = s.len;
    const v_q: TomlCodecVec16 = @splat('"');
    const v_bs: TomlCodecVec16 = @splat('\\');
    const v_lf: TomlCodecVec16 = @splat('\n');
    const v_cr: TomlCodecVec16 = @splat('\r');
    while (i + 16 <= n) {
        const chunk: TomlCodecVec16 = s[i..][0..16].*;
        const m_q: u16 = @bitCast(chunk == v_q);
        const m_b: u16 = @bitCast(chunk == v_bs);
        const m_lf: u16 = @bitCast(chunk == v_lf);
        const m_cr: u16 = @bitCast(chunk == v_cr);
        const mask: u16 = m_q | m_b | m_lf | m_cr;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        const c = s[i];
        if (c == '"' || c == '\\' || c == '\n' || c == '\r') return i;
    }
    return n;
}

fn tomlSimdFindMlBasicSpecial(s: []const u8, start: usize): usize {
    let i: usize = start;
    const n = s.len;
    const v_q: TomlCodecVec16 = @splat('"');
    const v_bs: TomlCodecVec16 = @splat('\\');
    while (i + 16 <= n) {
        const chunk: TomlCodecVec16 = s[i..][0..16].*;
        const m_q: u16 = @bitCast(chunk == v_q);
        const m_b: u16 = @bitCast(chunk == v_bs);
        const mask: u16 = m_q | m_b;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        const c = s[i];
        if (c == '"' || c == '\\') return i;
    }
    return n;
}

fn tomlSimdFindLiteralStrSpecial(s: []const u8, start: usize): usize {
    let i: usize = start;
    const n = s.len;
    const v_q: TomlCodecVec16 = @splat('\'');
    const v_lf: TomlCodecVec16 = @splat('\n');
    const v_cr: TomlCodecVec16 = @splat('\r');
    while (i + 16 <= n) {
        const chunk: TomlCodecVec16 = s[i..][0..16].*;
        const m_q: u16 = @bitCast(chunk == v_q);
        const m_lf: u16 = @bitCast(chunk == v_lf);
        const m_cr: u16 = @bitCast(chunk == v_cr);
        const mask: u16 = m_q | m_lf | m_cr;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        const c = s[i];
        if (c == '\'' || c == '\n' || c == '\r') return i;
    }
    return n;
}

fn tomlSimdFindByte(s: []const u8, start: usize, target: u8): usize {
    let i: usize = start;
    const n = s.len;
    const v_t: TomlCodecVec16 = @splat(target);
    while (i + 16 <= n) {
        const chunk: TomlCodecVec16 = s[i..][0..16].*;
        const mask: u16 = @bitCast(chunk == v_t);
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n && s[i] != target) : (i += 1) {}
    return i;
}

fn tomlSimdFindEscSpecial(s: []const u8, start: usize): usize {
    let i: usize = start;
    const n = s.len;
    const v_q: TomlCodecVec16 = @splat('"');
    const v_bs: TomlCodecVec16 = @splat('\\');
    const v_ctrl: TomlCodecVec16 = @splat(0x20);
    const v_del: TomlCodecVec16 = @splat(0x7F);
    while (i + 16 <= n) {
        const chunk: TomlCodecVec16 = s[i..][0..16].*;
        const m_q: u16 = @bitCast(chunk == v_q);
        const m_b: u16 = @bitCast(chunk == v_bs);
        const m_c: u16 = @bitCast(chunk < v_ctrl);
        const m_d: u16 = @bitCast(chunk == v_del);
        const mask: u16 = m_q | m_b | m_c | m_d;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        const c = s[i];
        if (c == '"' || c == '\\' || c < 0x20 || c == 0x7F) return i;
    }
    return n;
}

pub struct TomlParser {
    allocator: std.mem.Allocator,
    src: []const u8,
    pos: usize = 0,
    depth: usize = 0,

    fn init(*_, alloc: std.mem.Allocator, src: []const u8) {
        _.allocator = alloc;
        _.src = src;
        _.pos = 0;
        _.depth = 0;
    }

    static fn parse(alloc: std.mem.Allocator, src: []const u8): TomlDoc! {
        let parser = TomlParser{
            allocator: alloc,
            src: src,
            pos: 0,
            depth: 0,
        };
        return parser.parseDoc();
    }

    fn parseDoc(*_): TomlDoc! {
        let doc = TomlDoc.init(_.allocator);
        errdefer doc.deinit();

        // Strip UTF-8 BOM if present
        if (_.src.len >= 3 && _.src[0] == 0xEF && _.src[1] == 0xBB && _.src[2] == 0xBF) {
            _.pos = 3;
        }

        let current_table: *TomlTable! = unsafe { *doc.root! };

        while (_.pos < _.src.len) {
            _.skipWsAndComments();
            if (_.pos >= _.src.len) break;

            const c = _.src[_.pos];
            if (c == '\n' || c == '\r') {
                _.consumeNewline()?;
                continue;
            }

            if (c == '[') {
                current_table = _.parseTableHeader(&doc.root)?;
                continue;
            }

            // Key-value pair in current_table
            const keys = _.parseKeyPath()?;
            defer _.freeKeyPath(keys);

            _.skipWs();
            if (_.pos >= _.src.len || _.src[_.pos] != '=') {
                if (_.pos >= _.src.len) return TomlError.UnexpectedEof;
                return TomlError.UnexpectedChar;
            }
            _.pos += 1; // skip '='
            _.skipWs();

            let val = _.parseValue()?;
            let val_consumed = false;
            errdefer {
                if (!val_consumed) {
                    val.deinitWith(_.allocator);
                }
            }

            _.skipWsAndComments();
            if (_.pos < _.src.len) {
                const nc = _.src[_.pos];
                if (nc != '\n' && nc != '\r') {
                    return TomlError.UnexpectedChar;
                }
                _.consumeNewline()?;
            }

            _.setDottedKeyValue(current_table, keys, val)?;
            val_consumed = true;
        }

        return doc;
    }

    fn freeKeyPath(keys: [][]const u8) {
        for (k of keys) {
            _.allocator.free(k);
        }
        _.allocator.free(keys);
    }

    fn skipWs(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t') {
                _.pos += 1;
            } else {
                break;
            }
        }
    }

    fn skipWsAndComments(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t') {
                _.pos += 1;
            } else if (c == '#') {
                _.pos = tomlSimdFindEol(_.src, _.pos + 1);
                break;
            } else {
                break;
            }
        }
    }

    fn skipWsNewlinesAndComments(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                _.pos += 1;
            } else if (c == '#') {
                _.pos = tomlSimdFindEol(_.src, _.pos + 1);
            } else {
                break;
            }
        }
    }

    fn consumeNewline(*_): ! {
        if (_.pos >= _.src.len) return;
        if (_.src[_.pos] == '\r') {
            if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '\n') {
                _.pos += 2;
                return;
            }
            return TomlError.UnexpectedChar;
        }
        if (_.src[_.pos] == '\n') {
            _.pos += 1;
            return;
        }
        return TomlError.UnexpectedChar;
    }

    static fn isBareKeyChar(c: u8): bool {
        return (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '_' || c == '-';
    }

    fn parseSingleKey(*_): ([]const u8)! {
        if (_.pos >= _.src.len) return TomlError.UnexpectedEof;
        const c = _.src[_.pos];
        if (c == '"') {
            return _.parseBasicString();
        }
        if (c == '\'') {
            return _.parseLiteralString();
        }
        if (!TomlParser.isBareKeyChar(c)) {
            return TomlError.UnexpectedChar;
        }
        const start = _.pos;
        _.pos = tomlSimdSkipBareKey(_.src, _.pos);
        return _.allocator.dupe(u8, _.src[start.._.pos]) catch return TomlError.OutOfMemory;
    }

    fn parseKeyPath(*_): ([][]const u8)! {
        let list: std.ArrayList(string) = .empty;
        errdefer {
            for (k of list.items) {
                _.allocator.free(k);
            }
            list.deinit(_.allocator);
        }

        const first = _.parseSingleKey()?;
        list.append(_.allocator, first) catch {
            _.allocator.free(first);
            return TomlError.OutOfMemory;
        };

        while (true) {
            _.skipWs();
            if (_.pos < _.src.len && _.src[_.pos] == '.') {
                _.pos += 1;
                _.skipWs();
                const seg = _.parseSingleKey()?;
                list.append(_.allocator, seg) catch {
                    _.allocator.free(seg);
                    return TomlError.OutOfMemory;
                };
            } else {
                break;
            }
        }

        if (_.depth + list.items.len > TOML_MAX_DEPTH) {
            return TomlError.MaxDepthExceeded;
        }
        return list.toOwnedSlice(_.allocator) catch return TomlError.OutOfMemory;
    }

    fn parseTableHeader(*_, root: *TomlTable!): (*TomlTable!)! {
        _.pos += 1; // skip first '['
        let is_array_table = false;
        if (_.pos < _.src.len && _.src[_.pos] == '[') {
            is_array_table = true;
            _.pos += 1;
        }

        _.skipWs();
        const keys = _.parseKeyPath()?;
        defer _.freeKeyPath(keys);
        _.skipWs();

        if (is_array_table) {
            if (_.pos + 1 >= _.src.len) return TomlError.UnexpectedEof;
            if (_.src[_.pos] != ']' || _.src[_.pos + 1] != ']') {
                return TomlError.UnexpectedChar;
            }
            _.pos += 2;
        } else {
            if (_.pos >= _.src.len) return TomlError.UnexpectedEof;
            if (_.src[_.pos] != ']') return TomlError.UnexpectedChar;
            _.pos += 1;
        }

        _.skipWsAndComments();
        if (_.pos < _.src.len) {
            const nc = _.src[_.pos];
            if (nc != '\n' && nc != '\r') {
                return TomlError.UnexpectedChar;
            }
            _.consumeNewline()?;
        }

        return _.resolveTableForHeader(root, keys, is_array_table);
    }

    fn resolveTableForHeader(*_, root: *TomlTable!, keys: []const []const u8, is_array_table: bool): (*TomlTable!)! {
        let cur: *TomlTable! = root;
        let idx: usize = 0;

        while (idx + 1 < keys.len) : (idx += 1) {
            const seg = keys[idx];
            if (entry := cur.getEntryPtr(seg)) {
                if (entry.value.kind == .Table) {
                    if (entry.is_inline_table) {
                        return TomlError.TypeConflict;
                    }
                    cur = entry.value.asTablePtr();
                } else if (entry.value.kind == .Array && entry.is_array_of_tables) {
                    const arr_len = entry.value.arr_val.items.items.len;
                    if (arr_len == 0) return TomlError.TypeConflict;
                    const last_ptr = entry.value.asArrayPtr().getPtr(arr_len - 1) orelse return TomlError.TypeConflict;
                    if (last_ptr.kind != .Table) return TomlError.TypeConflict;
                    cur = last_ptr.asTablePtr();
                } else {
                    return TomlError.TypeConflict;
                }
            } else {
                const key_copy = _.allocator.dupe(u8, seg) catch return TomlError.OutOfMemory;
                errdefer _.allocator.free(key_copy);
                cur.appendEntryWith(_.allocator, TomlEntry{
                    key: key_copy,
                    value: TomlValue.fromTable(TomlTable.init(_.allocator)),
                    is_explicit_table: false,
                    is_dotted_table: false,
                }) catch return TomlError.OutOfMemory;
                const new_idx = cur.entries.items.len - 1;
                cur = cur.getEntryAtPtr(new_idx).value.asTablePtr();
            }
        }

        const last_key = keys[keys.len - 1];
        if (!is_array_table) {
            if (entry := cur.getEntryPtr(last_key)) {
                if (entry.value.kind != .Table) {
                    return TomlError.TypeConflict;
                }
                if (entry.is_inline_table) {
                    return TomlError.TypeConflict;
                }
                if (entry.is_explicit_table || entry.is_dotted_table) {
                    return TomlError.DuplicateKey;
                }
                entry.is_explicit_table = true;
                return entry.value.asTablePtr();
            } else {
                const key_copy = _.allocator.dupe(u8, last_key) catch return TomlError.OutOfMemory;
                errdefer _.allocator.free(key_copy);
                cur.appendEntryWith(_.allocator, TomlEntry{
                    key: key_copy,
                    value: TomlValue.fromTable(TomlTable.init(_.allocator)),
                    is_explicit_table: true,
                }) catch return TomlError.OutOfMemory;
                const new_idx = cur.entries.items.len - 1;
                return cur.getEntryAtPtr(new_idx).value.asTablePtr();
            }
        } else {
            if (entry := cur.getEntryPtr(last_key)) {
                if (entry.value.kind != .Array || !entry.is_array_of_tables) {
                    return TomlError.TypeConflict;
                }
                let new_tbl = TomlTable.init(_.allocator);
                errdefer new_tbl.deinit();
                entry.value.arr_val.pushWith(_.allocator, TomlValue.fromTable(new_tbl)) catch return TomlError.OutOfMemory;
                const last_i = entry.value.arr_val.items.items.len - 1;
                const elem_ptr = entry.value.asArrayPtr().getPtr(last_i) orelse return TomlError.TypeConflict;
                return elem_ptr.asTablePtr();
            } else {
                let arr = TomlArray.init(_.allocator);
                errdefer arr.deinit();
                let new_tbl = TomlTable.init(_.allocator);
                arr.pushWith(_.allocator, TomlValue.fromTable(new_tbl)) catch return TomlError.OutOfMemory;

                const key_copy = _.allocator.dupe(u8, last_key) catch return TomlError.OutOfMemory;
                errdefer _.allocator.free(key_copy);

                cur.appendEntryWith(_.allocator, TomlEntry{
                    key: key_copy,
                    value: TomlValue.fromArray(arr),
                    is_array_of_tables: true,
                }) catch return TomlError.OutOfMemory;
                const new_idx = cur.entries.items.len - 1;
                const elem_ptr = cur.getEntryAtPtr(new_idx).value.asArrayPtr().getPtr(0) orelse return TomlError.TypeConflict;
                return elem_ptr.asTablePtr();
            }
        }
    }

    fn setDottedKeyValue(*_, target: *TomlTable!, keys: []const []const u8, val: TomlValue): ! {
        let cur: *TomlTable! = target;
        let idx: usize = 0;

        while (idx + 1 < keys.len) : (idx += 1) {
            const seg = keys[idx];
            if (entry := cur.getEntryPtr(seg)) {
                if (entry.value.kind != .Table) {
                    return TomlError.TypeConflict;
                }
                if (entry.is_inline_table) {
                    return TomlError.TypeConflict;
                }
                if (entry.is_explicit_table) {
                    return TomlError.DuplicateKey;
                }
                cur = entry.value.asTablePtr();
            } else {
                const key_copy = _.allocator.dupe(u8, seg) catch return TomlError.OutOfMemory;
                errdefer _.allocator.free(key_copy);
                cur.appendEntryWith(_.allocator, TomlEntry{
                    key: key_copy,
                    value: TomlValue.fromTable(TomlTable.init(_.allocator)),
                    is_dotted_table: true,
                }) catch return TomlError.OutOfMemory;
                const new_idx = cur.entries.items.len - 1;
                cur = cur.getEntryAtPtr(new_idx).value.asTablePtr();
            }
        }

        const last_key = keys[keys.len - 1];
        if (cur.contains(last_key)) {
            return TomlError.DuplicateKey;
        }
        const key_copy = _.allocator.dupe(u8, last_key) catch return TomlError.OutOfMemory;
        errdefer _.allocator.free(key_copy);

        cur.appendEntryWith(_.allocator, TomlEntry{
            key: key_copy,
            value: val,
            is_inline_table: val.kind == .Table,
        }) catch return TomlError.OutOfMemory;
    }

    fn parseValue(*_): TomlValue! {
        if (_.pos >= _.src.len) return TomlError.UnexpectedEof;
        const c = _.src[_.pos];

        if (c == '"') {
            if (_.pos + 2 < _.src.len && _.src[_.pos + 1] == '"' && _.src[_.pos + 2] == '"') {
                const s = _.parseMultilineBasicString()?;
                return TomlValue.fromStringOwned(s);
            }
            const s = _.parseBasicString()?;
            return TomlValue.fromStringOwned(s);
        }

        if (c == '\'') {
            if (_.pos + 2 < _.src.len && _.src[_.pos + 1] == '\'' && _.src[_.pos + 2] == '\'') {
                const s = _.parseMultilineLiteralString()?;
                return TomlValue.fromStringOwned(s);
            }
            const s = _.parseLiteralString()?;
            return TomlValue.fromStringOwned(s);
        }

        if (c == '[') {
            return _.parseArray();
        }

        if (c == '{') {
            return _.parseInlineTable();
        }

        if (c == 't' || c == 'f') {
            return _.parseBooleanOrError();
        }

        if (_.looksLikeDateTime()) {
            const dt = _.parseDateTime()?;
            return TomlValue.fromDateTimeOwned(dt);
        }

        if (c == '+' || c == '-' || c == 'i' || c == 'n' || (c >= '0' && c <= '9')) {
            return _.parseNumberOrSpecialFloat();
        }

        return TomlError.UnexpectedChar;
    }

    static fn isValueDelimiter(c: u8): bool {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '#' || c == ',' || c == ']' || c == '}';
    }

    fn parseBooleanOrError(*_): TomlValue! {
        if (_.pos + 4 <= _.src.len && std.mem.eql(u8, _.src[_.pos.._.pos + 4], "true")) {
            if (_.pos + 4 == _.src.len || TomlParser.isValueDelimiter(_.src[_.pos + 4])) {
                _.pos += 4;
                return TomlValue.fromBool(true);
            }
        }
        if (_.pos + 5 <= _.src.len && std.mem.eql(u8, _.src[_.pos.._.pos + 5], "false")) {
            if (_.pos + 5 == _.src.len || TomlParser.isValueDelimiter(_.src[_.pos + 5])) {
                _.pos += 5;
                return TomlValue.fromBool(false);
            }
        }
        return TomlError.UnexpectedChar;
    }

    fn parseEscape(*_, out: *std.ArrayList(u8)): ! {
        if (_.pos >= _.src.len) return TomlError.UnterminatedString;
        const esc = _.src[_.pos];
        _.pos += 1;

        switch (esc) {
            'b' => out.append(_.allocator, 0x08) catch return TomlError.OutOfMemory,
            't' => out.append(_.allocator, '\t') catch return TomlError.OutOfMemory,
            'n' => out.append(_.allocator, '\n') catch return TomlError.OutOfMemory,
            'f' => out.append(_.allocator, 0x0C) catch return TomlError.OutOfMemory,
            'r' => out.append(_.allocator, '\r') catch return TomlError.OutOfMemory,
            '"' => out.append(_.allocator, '"') catch return TomlError.OutOfMemory,
            '\\' => out.append(_.allocator, '\\') catch return TomlError.OutOfMemory,
            'u' => _.parseUnicodeEscape(out, 4)?,
            'U' => _.parseUnicodeEscape(out, 8)?,
            else => return TomlError.InvalidEscape,
        }
    }

    fn parseUnicodeEscape(*_, out: *std.ArrayList(u8), digits: usize): ! {
        if (_.pos + digits > _.src.len) return TomlError.InvalidEscape;
        let codepoint: u32 = 0;
        let i: usize = 0;
        while (i < digits) : (i += 1) {
            const ch = _.src[_.pos + i];
            let d: u32 = 0;
            if (ch >= '0' && ch <= '9') {
                d = u32(ch - '0');
            } else if (ch >= 'a' && ch <= 'f') {
                d = u32(ch - 'a' + 10);
            } else if (ch >= 'A' && ch <= 'F') {
                d = u32(ch - 'A' + 10);
            } else {
                return TomlError.InvalidEscape;
            }
            codepoint = (codepoint << 4) | d;
        }
        _.pos += digits;

        if (codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
            return TomlError.InvalidEscape;
        }
        let utf8_buf: [4]u8 = undefined;
        const n = std.unicode.utf8Encode(@intCast(codepoint), &utf8_buf) catch return TomlError.InvalidEscape;
        out.appendSlice(_.allocator, utf8_buf[0..n]) catch return TomlError.OutOfMemory;
    }

    fn parseBasicString(*_): ([]const u8)! {
        _.pos += 1; // skip opening '"'
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(_.allocator);

        while (_.pos < _.src.len) {
            const hit = tomlSimdFindBasicStrSpecial(_.src, _.pos);
            if (hit > _.pos) {
                out.appendSlice(_.allocator, _.src[_.pos..hit]) catch return TomlError.OutOfMemory;
                _.pos = hit;
            }
            if (_.pos >= _.src.len) break;
            const c = _.src[_.pos];
            if (c == '"') {
                _.pos += 1;
                return out.toOwnedSlice(_.allocator) catch return TomlError.OutOfMemory;
            }
            if (c == '\n' || c == '\r') {
                return TomlError.UnterminatedString;
            }
            // c == '\\'
            _.pos += 1;
            _.parseEscape(&out)?;
        }
        return TomlError.UnterminatedString;
    }

    fn parseMultilineBasicString(*_): ([]const u8)! {
        _.pos += 3; // skip '"""'
        // Trim immediate newline after opening delimiter
        if (_.pos < _.src.len && _.src[_.pos] == '\r' && _.pos + 1 < _.src.len && _.src[_.pos + 1] == '\n') {
            _.pos += 2;
        } else if (_.pos < _.src.len && _.src[_.pos] == '\n') {
            _.pos += 1;
        }

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(_.allocator);

        while (_.pos < _.src.len) {
            const hit = tomlSimdFindMlBasicSpecial(_.src, _.pos);
            if (hit > _.pos) {
                out.appendSlice(_.allocator, _.src[_.pos..hit]) catch return TomlError.OutOfMemory;
                _.pos = hit;
            }
            if (_.pos >= _.src.len) break;
            const c = _.src[_.pos];
            if (c == '"') {
                let q_count: usize = 0;
                while (_.pos + q_count < _.src.len && _.src[_.pos + q_count] == '"' && q_count < 6) {
                    q_count += 1;
                }
                if (q_count >= 3) {
                    if (q_count > 5) return TomlError.UnexpectedChar;
                    const extra = q_count - 3;
                    let k: usize = 0;
                    while (k < extra) : (k += 1) {
                        out.append(_.allocator, '"') catch return TomlError.OutOfMemory;
                    }
                    _.pos += q_count;
                    return out.toOwnedSlice(_.allocator) catch return TomlError.OutOfMemory;
                }
                out.append(_.allocator, '"') catch return TomlError.OutOfMemory;
                _.pos += 1;
                continue;
            }

            // c == '\\'
            _.pos += 1;
            if (_.pos >= _.src.len) return TomlError.UnterminatedString;

            // Check line-ending backslash (trim whitespace + newlines)
            let look = _.pos;
            while (look < _.src.len && (_.src[look] == ' ' || _.src[look] == '\t')) {
                look += 1;
            }
            if (look < _.src.len && (_.src[look] == '\n' || _.src[look] == '\r')) {
                _.pos = look;
                while (_.pos < _.src.len) {
                    const wc = _.src[_.pos];
                    if (wc == ' ' || wc == '\t' || wc == '\n' || wc == '\r') {
                        _.pos += 1;
                    } else {
                        break;
                    }
                }
                continue;
            }

            _.parseEscape(&out)?;
        }
        return TomlError.UnterminatedString;
    }

    fn parseLiteralString(*_): ([]const u8)! {
        _.pos += 1; // skip opening '\''
        const start = _.pos;
        _.pos = tomlSimdFindLiteralStrSpecial(_.src, _.pos);
        if (_.pos < _.src.len && _.src[_.pos] == '\'') {
            const s = _.src[start.._.pos];
            _.pos += 1;
            return _.allocator.dupe(u8, s) catch return TomlError.OutOfMemory;
        }
        return TomlError.UnterminatedString;
    }

    fn parseMultilineLiteralString(*_): ([]const u8)! {
        _.pos += 3; // skip "'''"
        if (_.pos < _.src.len && _.src[_.pos] == '\r' && _.pos + 1 < _.src.len && _.src[_.pos + 1] == '\n') {
            _.pos += 2;
        } else if (_.pos < _.src.len && _.src[_.pos] == '\n') {
            _.pos += 1;
        }

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(_.allocator);

        while (_.pos < _.src.len) {
            const hit = tomlSimdFindByte(_.src, _.pos, '\'');
            if (hit > _.pos) {
                out.appendSlice(_.allocator, _.src[_.pos..hit]) catch return TomlError.OutOfMemory;
                _.pos = hit;
            }
            if (_.pos >= _.src.len) break;
            let q_count: usize = 0;
            while (_.pos + q_count < _.src.len && _.src[_.pos + q_count] == '\'' && q_count < 6) {
                q_count += 1;
            }
            if (q_count >= 3) {
                if (q_count > 5) return TomlError.UnexpectedChar;
                const extra = q_count - 3;
                let k: usize = 0;
                while (k < extra) : (k += 1) {
                    out.append(_.allocator, '\'') catch return TomlError.OutOfMemory;
                }
                _.pos += q_count;
                return out.toOwnedSlice(_.allocator) catch return TomlError.OutOfMemory;
            }
            out.append(_.allocator, '\'') catch return TomlError.OutOfMemory;
            _.pos += 1;
        }
        return TomlError.UnterminatedString;
    }

    fn parseArray(*_): TomlValue! {
        if (_.depth + 1 > TOML_MAX_DEPTH) return TomlError.MaxDepthExceeded;
        _.depth += 1;
        defer { _.depth -= 1; }

        _.pos += 1; // skip '['
        let arr = TomlArray.init(_.allocator);
        errdefer arr.deinit();

        while (true) {
            _.skipWsNewlinesAndComments();
            if (_.pos >= _.src.len) return TomlError.UnexpectedEof;
            if (_.src[_.pos] == ']') {
                _.pos += 1;
                return TomlValue.fromArray(arr);
            }

            let elem = _.parseValue()?;
            let elem_consumed = false;
            errdefer {
                if (!elem_consumed) {
                    elem.deinitWith(_.allocator);
                }
            }
            arr.pushWith(_.allocator, elem) catch return TomlError.OutOfMemory;
            elem_consumed = true;

            _.skipWsNewlinesAndComments();
            if (_.pos >= _.src.len) return TomlError.UnexpectedEof;
            if (_.src[_.pos] == ',') {
                _.pos += 1;
                continue;
            }
            if (_.src[_.pos] == ']') {
                _.pos += 1;
                return TomlValue.fromArray(arr);
            }
            return TomlError.UnexpectedChar;
        }
    }

    fn parseInlineTable(*_): TomlValue! {
        if (_.depth + 1 > TOML_MAX_DEPTH) return TomlError.MaxDepthExceeded;
        _.depth += 1;
        defer { _.depth -= 1; }

        _.pos += 1; // skip '{'
        let tbl = TomlTable.init(_.allocator);
        errdefer tbl.deinit();

        _.skipWs();
        if (_.pos >= _.src.len) return TomlError.UnexpectedEof;
        if (_.src[_.pos] == '}') {
            _.pos += 1;
            return TomlValue.fromTable(tbl);
        }

        while (true) {
            _.skipWs();
            if (_.pos >= _.src.len) return TomlError.UnexpectedEof;
            if (_.src[_.pos] == '\n' || _.src[_.pos] == '\r') {
                return TomlError.UnexpectedChar;
            }

            const keys = _.parseKeyPath()?;
            defer _.freeKeyPath(keys);

            _.skipWs();
            if (_.pos >= _.src.len || _.src[_.pos] != '=') return TomlError.UnexpectedChar;
            _.pos += 1;
            _.skipWs();

            let val = _.parseValue()?;
            let val_consumed = false;
            errdefer {
                if (!val_consumed) {
                    val.deinitWith(_.allocator);
                }
            }
            _.setDottedKeyValue(&tbl, keys, val)?;
            val_consumed = true;

            _.skipWs();
            if (_.pos >= _.src.len) return TomlError.UnexpectedEof;
            const c = _.src[_.pos];
            if (c == '}') {
                _.pos += 1;
                return TomlValue.fromTable(tbl);
            }
            if (c == ',') {
                _.pos += 1;
                _.skipWs();
                if (_.pos < _.src.len && _.src[_.pos] == '}') {
                    return TomlError.UnexpectedChar; // trailing comma forbidden in inline table
                }
                continue;
            }
            return TomlError.UnexpectedChar;
        }
    }

    static fn isDigit(c: u8): bool {
        return c >= '0' && c <= '9';
    }

    fn looksLikeDateTime(): bool {
        // YYYY-MM-DD...
        if (_.pos + 10 <= _.src.len) {
            const s = _.src[_.pos.._.pos + 10];
            if (TomlParser.isDigit(s[0]) && TomlParser.isDigit(s[1]) && TomlParser.isDigit(s[2]) && TomlParser.isDigit(s[3]) &&
                s[4] == '-' && TomlParser.isDigit(s[5]) && TomlParser.isDigit(s[6]) &&
                s[7] == '-' && TomlParser.isDigit(s[8]) && TomlParser.isDigit(s[9]))
            {
                return true;
            }
        }
        // HH:MM:SS...
        if (_.pos + 8 <= _.src.len) {
            const s = _.src[_.pos.._.pos + 8];
            if (TomlParser.isDigit(s[0]) && TomlParser.isDigit(s[1]) && s[2] == ':' &&
                TomlParser.isDigit(s[3]) && TomlParser.isDigit(s[4]) && s[5] == ':' &&
                TomlParser.isDigit(s[6]) && TomlParser.isDigit(s[7]))
            {
                return true;
            }
        }
        return false;
    }

    static fn parse2Digits(s: []const u8): u8? {
        if (s.len < 2 || !TomlParser.isDigit(s[0]) || !TomlParser.isDigit(s[1])) return null;
        return (s[0] - '0') * 10 + (s[1] - '0');
    }

    fn parseDateTime(*_): ([]const u8)! {
        const start = _.pos;
        let p = _.pos;

        let has_date = false;
        let has_time = false;

        // Check date YYYY-MM-DD
        if (p + 10 <= _.src.len && _.src[p + 4] == '-' && _.src[p + 7] == '-') {
            const month = TomlParser.parse2Digits(_.src[p + 5..p + 7]) orelse return TomlError.InvalidDateTime;
            const day = TomlParser.parse2Digits(_.src[p + 8..p + 10]) orelse return TomlError.InvalidDateTime;
            if (month < 1 || month > 12 || day < 1 || day > 31) return TomlError.InvalidDateTime;
            p += 10;
            has_date = true;

            if (p < _.src.len) {
                const sep = _.src[p];
                if (sep == 'T' || sep == 't') {
                    p += 1;
                    has_time = true;
                } else if (sep == ' ' && p + 9 <= _.src.len &&
                    TomlParser.isDigit(_.src[p + 1]) && TomlParser.isDigit(_.src[p + 2]) && _.src[p + 3] == ':')
                {
                    p += 1;
                    has_time = true;
                }
            }
        } else {
            has_time = true;
        }

        if (has_time) {
            if (p + 8 > _.src.len || _.src[p + 2] != ':' || _.src[p + 5] != ':') {
                return TomlError.InvalidDateTime;
            }
            const hour = TomlParser.parse2Digits(_.src[p..p + 2]) orelse return TomlError.InvalidDateTime;
            const min = TomlParser.parse2Digits(_.src[p + 3..p + 5]) orelse return TomlError.InvalidDateTime;
            const sec = TomlParser.parse2Digits(_.src[p + 6..p + 8]) orelse return TomlError.InvalidDateTime;
            if (hour > 23 || min > 59 || sec > 60) return TomlError.InvalidDateTime;
            p += 8;

            // Optional fractional seconds
            if (p < _.src.len && _.src[p] == '.') {
                p += 1;
                if (p >= _.src.len || !TomlParser.isDigit(_.src[p])) return TomlError.InvalidDateTime;
                while (p < _.src.len && TomlParser.isDigit(_.src[p])) {
                    p += 1;
                }
            }

            // Optional offset (only valid if date was present)
            if (has_date && p < _.src.len) {
                const zc = _.src[p];
                if (zc == 'Z' || zc == 'z') {
                    p += 1;
                } else if (zc == '+' || zc == '-') {
                    p += 1;
                    if (p + 5 > _.src.len || _.src[p + 2] != ':') return TomlError.InvalidDateTime;
                    const oh = TomlParser.parse2Digits(_.src[p..p + 2]) orelse return TomlError.InvalidDateTime;
                    const om = TomlParser.parse2Digits(_.src[p + 3..p + 5]) orelse return TomlError.InvalidDateTime;
                    if (oh > 23 || om > 59) return TomlError.InvalidDateTime;
                    p += 5;
                }
            }
        }

        if (p < _.src.len && !TomlParser.isValueDelimiter(_.src[p])) {
            return TomlError.InvalidDateTime;
        }

        _.pos = p;
        return _.allocator.dupe(u8, _.src[start..p]) catch return TomlError.OutOfMemory;
    }

    fn parseNumberOrSpecialFloat(*_): TomlValue! {
        const start = _.pos;
        while (_.pos < _.src.len && !TomlParser.isValueDelimiter(_.src[_.pos])) {
            _.pos += 1;
        }
        const token = _.src[start.._.pos];
        if (token.len == 0) return TomlError.InvalidNumber;

        // Special float literals
        if (std.mem.eql(u8, token, "inf") || std.mem.eql(u8, token, "+inf")) {
            return TomlValue.fromFloat(std.math.inf(f64));
        }
        if (std.mem.eql(u8, token, "-inf")) {
            return TomlValue.fromFloat(0.0 - std.math.inf(f64));
        }
        if (std.mem.eql(u8, token, "nan") || std.mem.eql(u8, token, "+nan") || std.mem.eql(u8, token, "-nan")) {
            return TomlValue.fromFloat(std.math.nan(f64));
        }

        // Prefixed integers: 0x, 0o, 0b
        if (token.len >= 3 && token[0] == '0' && (token[1] == 'x' || token[1] == 'o' || token[1] == 'b')) {
            const radix: u8 = switch (token[1]) {
                'x' => 16,
                'o' => 8,
                else => 2,
            };
            const body = token[2..];
            if (body.len == 0 || body[0] == '_' || body[body.len - 1] == '_') {
                return TomlError.InvalidNumber;
            }
            let clean: std.ArrayList(u8) = .empty;
            defer clean.deinit(_.allocator);
            let prev_under = false;
            for (ch of body) {
                if (ch == '_') {
                    if (prev_under) return TomlError.InvalidNumber;
                    prev_under = true;
                    continue;
                }
                prev_under = false;
                clean.append(_.allocator, ch) catch return TomlError.OutOfMemory;
            }
            const parsed = std.fmt.parseInt(i64, clean.items, radix) catch return TomlError.InvalidNumber;
            return TomlValue.fromInt(parsed);
        }

        // Check if float (contains '.' or 'e' or 'E')
        let is_float = false;
        for (ch of token) {
            if (ch == '.' || ch == 'e' || ch == 'E') {
                is_float = true;
                break;
            }
        }

        // Strip and validate underscores & leading zero rules
        let idx: usize = 0;
        let clean: std.ArrayList(u8) = .empty;
        defer clean.deinit(_.allocator);

        if (token[0] == '+' || token[0] == '-') {
            clean.append(_.allocator, token[0]) catch return TomlError.OutOfMemory;
            idx = 1;
            if (idx >= token.len) return TomlError.InvalidNumber;
        }

        const mag = token[idx..];
        if (mag.len == 0 || mag[0] == '_' || mag[mag.len - 1] == '_') {
            return TomlError.InvalidNumber;
        }
        // Leading zero check on integer part
        if (mag.len > 1 && mag[0] == '0' && mag[1] != '.' && mag[1] != 'e' && mag[1] != 'E') {
            return TomlError.InvalidNumber;
        }

        let prev_under = false;
        let i: usize = 0;
        while (i < mag.len) : (i += 1) {
            const ch = mag[i];
            if (ch == '_') {
                if (prev_under) return TomlError.InvalidNumber;
                if (i == 0 || i + 1 >= mag.len) return TomlError.InvalidNumber;
                const prev_c = mag[i - 1];
                const next_c = mag[i + 1];
                if (!TomlParser.isDigit(prev_c) || !TomlParser.isDigit(next_c)) {
                    return TomlError.InvalidNumber;
                }
                prev_under = true;
                continue;
            }
            prev_under = false;
            if (ch == '.') {
                if (i == 0 || i + 1 >= mag.len || !TomlParser.isDigit(mag[i - 1]) || !TomlParser.isDigit(mag[i + 1])) {
                    return TomlError.InvalidNumber;
                }
            }
            clean.append(_.allocator, ch) catch return TomlError.OutOfMemory;
        }

        if (is_float) {
            const fval = std.fmt.parseFloat(f64, clean.items) catch return TomlError.InvalidNumber;
            return TomlValue.fromFloat(fval);
        } else {
            const ival = std.fmt.parseInt(i64, clean.items, 10) catch return TomlError.InvalidNumber;
            return TomlValue.fromInt(ival);
        }
    }
}

pub static class TomlSerializer {
    fn isBareKey(key: []const u8): bool {
        if (key.len == 0) return false;
        return tomlSimdSkipBareKey(key, 0) == key.len;
    }

    fn writeQuotedString(out: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): ! {
        out.append(alloc, '"') catch return TomlError.OutOfMemory;
        let i: usize = 0;
        while (i < s.len) {
            const hit = tomlSimdFindEscSpecial(s, i);
            if (hit > i) {
                out.appendSlice(alloc, s[i..hit]) catch return TomlError.OutOfMemory;
                i = hit;
            }
            if (i >= s.len) break;
            const c = s[i];
            switch (c) {
                '"' => out.appendSlice(alloc, "\\\"") catch return TomlError.OutOfMemory,
                '\\' => out.appendSlice(alloc, "\\\\") catch return TomlError.OutOfMemory,
                '\n' => out.appendSlice(alloc, "\\n") catch return TomlError.OutOfMemory,
                '\r' => out.appendSlice(alloc, "\\r") catch return TomlError.OutOfMemory,
                '\t' => out.appendSlice(alloc, "\\t") catch return TomlError.OutOfMemory,
                0x08 => out.appendSlice(alloc, "\\b") catch return TomlError.OutOfMemory,
                0x0C => out.appendSlice(alloc, "\\f") catch return TomlError.OutOfMemory,
                else => {
                    let buf: [8]u8 = undefined;
                    const hex = std.fmt.bufPrint(&buf, "\\u{X:0>4}", .{c}) catch return TomlError.OutOfMemory;
                    out.appendSlice(alloc, hex) catch return TomlError.OutOfMemory;
                },
            }
            i += 1;
        }
        out.append(alloc, '"') catch return TomlError.OutOfMemory;
    }

    fn writeKey(out: *std.ArrayList(u8), alloc: std.mem.Allocator, key: []const u8): ! {
        if (TomlSerializer.isBareKey(key)) {
            out.appendSlice(alloc, key) catch return TomlError.OutOfMemory;
        } else {
            TomlSerializer.writeQuotedString(out, alloc, key)?;
        }
    }

    fn writeInlineValue(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: TomlValue): ! {
        switch (val.kind) {
            .String => TomlSerializer.writeQuotedString(out, alloc, val.str_val)?,
            .DateTime => out.appendSlice(alloc, val.str_val) catch return TomlError.OutOfMemory,
            .Integer => {
                let buf: [32]u8 = undefined;
                const s = std.fmt.bufPrint(&buf, "{d}", .{val.int_val}) catch return TomlError.OutOfMemory;
                out.appendSlice(alloc, s) catch return TomlError.OutOfMemory;
            },
            .Float => {
                if (std.math.isNan(val.float_val)) {
                    out.appendSlice(alloc, "nan") catch return TomlError.OutOfMemory;
                } else if (std.math.isPositiveInf(val.float_val)) {
                    out.appendSlice(alloc, "inf") catch return TomlError.OutOfMemory;
                } else if (std.math.isNegativeInf(val.float_val)) {
                    out.appendSlice(alloc, "-inf") catch return TomlError.OutOfMemory;
                } else {
                    let buf: [64]u8 = undefined;
                    const s = std.fmt.bufPrint(&buf, "{d}", .{val.float_val}) catch return TomlError.OutOfMemory;
                    out.appendSlice(alloc, s) catch return TomlError.OutOfMemory;
                    let has_dot_or_e = false;
                    for (ch of s) {
                        if (ch == '.' || ch == 'e' || ch == 'E') {
                            has_dot_or_e = true;
                            break;
                        }
                    }
                    if (!has_dot_or_e) {
                        out.appendSlice(alloc, ".0") catch return TomlError.OutOfMemory;
                    }
                }
            },
            .Boolean => {
                out.appendSlice(alloc, if (val.bool_val) "true" else "false") catch return TomlError.OutOfMemory;
            },
            .Array => {
                out.append(alloc, '[') catch return TomlError.OutOfMemory;
                let idx: usize = 0;
                for (item of val.arr_val.items.items) {
                    if (idx > 0) out.appendSlice(alloc, ", ") catch return TomlError.OutOfMemory;
                    TomlSerializer.writeInlineValue(out, alloc, item)?;
                    idx += 1;
                }
                out.append(alloc, ']') catch return TomlError.OutOfMemory;
            },
            .Table => {
                out.append(alloc, '{') catch return TomlError.OutOfMemory;
                let idx: usize = 0;
                for (entry of val.table_val.entries.items) {
                    if (idx > 0) out.appendSlice(alloc, ", ") catch return TomlError.OutOfMemory;
                    TomlSerializer.writeKey(out, alloc, entry.key)?;
                    out.appendSlice(alloc, " = ") catch return TomlError.OutOfMemory;
                    TomlSerializer.writeInlineValue(out, alloc, entry.value)?;
                    idx += 1;
                }
                out.append(alloc, '}') catch return TomlError.OutOfMemory;
            },
        }
    }

    fn isArrayOfTablesEntry(entry: TomlEntry): bool {
        if (entry.value.kind != .Array) return false;
        if (entry.is_array_of_tables) return true;
        if (entry.value.arr_val.items.items.len == 0) return false;
        for (item of entry.value.arr_val.items.items) {
            if (item.kind != .Table) return false;
        }
        return true;
    }

    fn writeTableSection(
        out: *std.ArrayList(u8),
        alloc: std.mem.Allocator,
        tbl: TomlTable,
        prefix: []const u8,
        is_root: bool,
    ): ! {
        // 1. Emit scalar/normal-array entries first
        for (entry of tbl.entries.items) {
            if (entry.value.kind == .Table) continue;
            if (TomlSerializer.isArrayOfTablesEntry(entry)) continue;

            TomlSerializer.writeKey(out, alloc, entry.key)?;
            out.appendSlice(alloc, " = ") catch return TomlError.OutOfMemory;
            TomlSerializer.writeInlineValue(out, alloc, entry.value)?;
            out.append(alloc, '\n') catch return TomlError.OutOfMemory;
        }

        // 2. Emit sub-tables
        for (entry of tbl.entries.items) {
            if (entry.value.kind != .Table) continue;

            let full_path: std.ArrayList(u8) = .empty;
            defer full_path.deinit(alloc);
            if (!is_root && prefix.len > 0) {
                full_path.appendSlice(alloc, prefix) catch return TomlError.OutOfMemory;
                full_path.append(alloc, '.') catch return TomlError.OutOfMemory;
            }
            TomlSerializer.writeKey(&full_path, alloc, entry.key)?;

            if (out.items.len > 0 && out.items[out.items.len - 1] != '\n') {
                out.append(alloc, '\n') catch return TomlError.OutOfMemory;
            }
            if (out.items.len > 0) {
                out.append(alloc, '\n') catch return TomlError.OutOfMemory;
            }
            out.append(alloc, '[') catch return TomlError.OutOfMemory;
            out.appendSlice(alloc, full_path.items) catch return TomlError.OutOfMemory;
            out.appendSlice(alloc, "]\n") catch return TomlError.OutOfMemory;

            TomlSerializer.writeTableSection(out, alloc, entry.value.table_val, full_path.items, false)?;
        }

        // 3. Emit array of tables
        for (entry of tbl.entries.items) {
            if (!TomlSerializer.isArrayOfTablesEntry(entry)) continue;

            let full_path: std.ArrayList(u8) = .empty;
            defer full_path.deinit(alloc);
            if (!is_root && prefix.len > 0) {
                full_path.appendSlice(alloc, prefix) catch return TomlError.OutOfMemory;
                full_path.append(alloc, '.') catch return TomlError.OutOfMemory;
            }
            TomlSerializer.writeKey(&full_path, alloc, entry.key)?;

            for (item of entry.value.arr_val.items.items) {
                if (item.kind != .Table) continue;
                if (out.items.len > 0) {
                    out.append(alloc, '\n') catch return TomlError.OutOfMemory;
                }
                out.appendSlice(alloc, "[[") catch return TomlError.OutOfMemory;
                out.appendSlice(alloc, full_path.items) catch return TomlError.OutOfMemory;
                out.appendSlice(alloc, "]]\n") catch return TomlError.OutOfMemory;
                TomlSerializer.writeTableSection(out, alloc, item.table_val, full_path.items, false)?;
            }
        }
    }

    pub fn stringify(alloc: std.mem.Allocator, doc: TomlDoc): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        TomlSerializer.writeTableSection(&out, alloc, doc.root, "", true)?;
        return out.toOwnedSlice(alloc) catch return TomlError.OutOfMemory;
    }

    fn writeJsonValue(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: TomlValue): ! {
        switch (val.kind) {
            .String | .DateTime => TomlSerializer.writeQuotedString(out, alloc, val.str_val)?,
            .Integer => {
                let buf: [32]u8 = undefined;
                const s = std.fmt.bufPrint(&buf, "{d}", .{val.int_val}) catch return TomlError.OutOfMemory;
                out.appendSlice(alloc, s) catch return TomlError.OutOfMemory;
            },
            .Float => {
                if (std.math.isNan(val.float_val) || std.math.isInf(val.float_val)) {
                    out.appendSlice(alloc, "null") catch return TomlError.OutOfMemory;
                } else {
                    let buf: [64]u8 = undefined;
                    const s = std.fmt.bufPrint(&buf, "{d}", .{val.float_val}) catch return TomlError.OutOfMemory;
                    out.appendSlice(alloc, s) catch return TomlError.OutOfMemory;
                }
            },
            .Boolean => {
                out.appendSlice(alloc, if (val.bool_val) "true" else "false") catch return TomlError.OutOfMemory;
            },
            .Array => {
                out.append(alloc, '[') catch return TomlError.OutOfMemory;
                let idx: usize = 0;
                for (item of val.arr_val.items.items) {
                    if (idx > 0) out.append(alloc, ',') catch return TomlError.OutOfMemory;
                    TomlSerializer.writeJsonValue(out, alloc, item)?;
                    idx += 1;
                }
                out.append(alloc, ']') catch return TomlError.OutOfMemory;
            },
            .Table => {
                out.append(alloc, '{') catch return TomlError.OutOfMemory;
                let idx: usize = 0;
                for (entry of val.table_val.entries.items) {
                    if (idx > 0) out.append(alloc, ',') catch return TomlError.OutOfMemory;
                    TomlSerializer.writeQuotedString(out, alloc, entry.key)?;
                    out.append(alloc, ':') catch return TomlError.OutOfMemory;
                    TomlSerializer.writeJsonValue(out, alloc, entry.value)?;
                    idx += 1;
                }
                out.append(alloc, '}') catch return TomlError.OutOfMemory;
            },
        }
    }

    pub fn toJson(alloc: std.mem.Allocator, doc: TomlDoc): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        TomlSerializer.writeJsonValue(&out, alloc, TomlValue.fromTable(doc.root))?;
        return out.toOwnedSlice(alloc) catch return TomlError.OutOfMemory;
    }
}
