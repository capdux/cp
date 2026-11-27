// Source: cp/lib/yaml/codec.cp (YAML 1.2 Parser, Anchor/Alias Resolver, Serializer & JSON Converter)
// Pure CP YAML 1.2 core subset parser and serializer with strict space indentation,
// duplicate-key rejection, 64-level depth guard, anchors/aliases, block scalars,
// flow collections, and RFC 8259 JSON export.

const std = @import("std");
import {
    YamlError,
    YamlKind,
    YamlEntry,
    YamlValue,
    YamlDoc,
} from "./value.cp";

const MAX_YAML_DEPTH: usize = 64;

type YamlCodecVec16 = @Vector(16, u8);

fn yamlSimdSkipSpaces(s: []const u8, start: usize): usize {
    let i: usize = start;
    const n = s.len;
    const v_sp: YamlCodecVec16 = @splat(' ');
    while (i + 16 <= n) {
        const chunk: YamlCodecVec16 = s[i..][0..16].*;
        const m_non: u16 = ~@as(u16, @bitCast(chunk == v_sp));
        if (m_non != 0) {
            return i + (@ctz(m_non) as usize);
        }
        i += 16;
    }
    while (i < n && s[i] == ' ') : (i += 1) {}
    return i;
}

fn yamlSimdFindEol(s: []const u8, start: usize): usize {
    let i: usize = start;
    const n = s.len;
    const v_lf: YamlCodecVec16 = @splat('\n');
    const v_cr: YamlCodecVec16 = @splat('\r');
    while (i + 16 <= n) {
        const chunk: YamlCodecVec16 = s[i..][0..16].*;
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

fn yamlSimdFindDqSpecial(s: []const u8, start: usize): usize {
    let i: usize = start;
    const n = s.len;
    const v_q: YamlCodecVec16 = @splat('"');
    const v_bs: YamlCodecVec16 = @splat('\\');
    const v_cr: YamlCodecVec16 = @splat('\r');
    while (i + 16 <= n) {
        const chunk: YamlCodecVec16 = s[i..][0..16].*;
        const m_q: u16 = @bitCast(chunk == v_q);
        const m_b: u16 = @bitCast(chunk == v_bs);
        const m_r: u16 = @bitCast(chunk == v_cr);
        const mask: u16 = m_q | m_b | m_r;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        const c = s[i];
        if (c == '"' || c == '\\' || c == '\r') return i;
    }
    return n;
}

fn yamlSimdFindSqSpecial(s: []const u8, start: usize): usize {
    let i: usize = start;
    const n = s.len;
    const v_q: YamlCodecVec16 = @splat('\'');
    const v_cr: YamlCodecVec16 = @splat('\r');
    while (i + 16 <= n) {
        const chunk: YamlCodecVec16 = s[i..][0..16].*;
        const m_q: u16 = @bitCast(chunk == v_q);
        const m_r: u16 = @bitCast(chunk == v_cr);
        const mask: u16 = m_q | m_r;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        const c = s[i];
        if (c == '\'' || c == '\r') return i;
    }
    return n;
}

fn yamlSimdFindQuoteEsc(s: []const u8, start: usize): usize {
    let i: usize = start;
    const n = s.len;
    const v_q: YamlCodecVec16 = @splat('"');
    const v_bs: YamlCodecVec16 = @splat('\\');
    const v_ctrl: YamlCodecVec16 = @splat(0x20);
    const v_del: YamlCodecVec16 = @splat(0x7F);
    while (i + 16 <= n) {
        const chunk: YamlCodecVec16 = s[i..][0..16].*;
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

struct AnchorEntry {
    name: []const u8 = "",
    value: YamlValue = .{},
}

struct LinePeek {
    is_eof: bool = false,
    indent: usize = 0,
    content_pos: usize = 0,
    is_doc_start: bool = false,
    is_doc_end: bool = false,
}

pub struct YamlParser {
    alloc: std.mem.Allocator,
    src: []const u8,
    pos: usize = 0,
    anchors: std.ArrayList(AnchorEntry) = .empty,

    static fn init(alloc: std.mem.Allocator, src: []const u8): YamlParser {
        let start: usize = 0;
        if (src.len >= 3 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) {
            start = 3;
        }
        return YamlParser{
            alloc: alloc,
            src: src,
            pos: start,
            anchors: .empty,
        };
    }

    fn deinit(*_) {
        for (*a of _.anchors.items) {
            if (a.name.len > 0) _.alloc.free(a.name);
            a.value.deinit(_.alloc);
        }
        _.anchors.deinit(_.alloc);
    }

    fn eof(): bool {
        return _.pos >= _.src.len;
    }

    fn peek(): u8 {
        return _.src[_.pos];
    }

    fn peekAt(idx: usize): u8? {
        if (idx >= _.src.len) return null;
        return _.src[idx];
    }

    fn isEolChar(ch: u8): bool {
        return ch == '\n' || ch == '\r';
    }

    fn isWsChar(ch: u8): bool {
        return ch == ' ' || ch == '\t';
    }

    fn skipHorizWs(*_) {
        while (_.pos < _.src.len && (_.src[_.pos] == ' ' || _.src[_.pos] == '\t')) {
            _.pos += 1;
        }
    }

    fn consumeNewlineAt(idx: usize): usize {
        if (idx >= _.src.len) return idx;
        if (_.src[idx] == '\r') {
            if (idx + 1 < _.src.len && _.src[idx + 1] == '\n') return idx + 2;
            return idx + 1;
        }
        if (_.src[idx] == '\n') return idx + 1;
        return idx;
    }

    fn skipToNextLine(*_) {
        _.pos = yamlSimdFindEol(_.src, _.pos);
        _.pos = _.consumeNewlineAt(_.pos);
    }

    fn dupStr(s: []const u8): ([]const u8)! {
        if (s.len == 0) return "";
        return _.alloc.dupe(u8, s) catch return YamlError.OutOfMemory;
    }

    fn registerAnchor(*_, name: []const u8, val: YamlValue): ! {
        const cloned = val.clone(_.alloc)?;
        errdefer {
            let tmp = cloned;
            tmp.deinit(_.alloc);
        }
        for (*a of _.anchors.items) {
            if (std.mem.eql(u8, a.name, name)) {
                a.value.deinit(_.alloc);
                a.value = cloned;
                return;
            }
        }
        const name_dup = _.dupStr(name)?;
        errdefer {
            if (name_dup.len > 0) _.alloc.free(name_dup);
        }
        _.anchors.append(_.alloc, AnchorEntry{ name: name_dup, value: cloned }) catch return YamlError.OutOfMemory;
    }

    fn resolveAlias(name: []const u8): YamlValue! {
        for (a of _.anchors.items) {
            if (std.mem.eql(u8, a.name, name)) {
                return a.value.clone(_.alloc);
            }
        }
        return YamlError.InvalidAlias;
    }

    fn isDocSepAt(idx: usize, ch: u8): bool {
        if (idx + 3 > _.src.len) return false;
        if (_.src[idx] != ch || _.src[idx + 1] != ch || _.src[idx + 2] != ch) return false;
        if (idx + 3 == _.src.len) return true;
        const next = _.src[idx + 3];
        return next == ' ' || next == '\t' || next == '\r' || next == '\n' || next == '#';
    }

    /// Advances past blank lines and full-line comments, validating no tab indentation.
    fn peekNextLine(*_): LinePeek! {
        while (_.pos < _.src.len) {
            let line_start = _.pos;
            let cur = yamlSimdSkipSpaces(_.src, line_start);
            let spaces: usize = cur - line_start;
            if (cur < _.src.len && _.src[cur] == '\t') {
                return YamlError.TabIndentationForbidden;
            }
            if (cur >= _.src.len) {
                _.pos = cur;
                return LinePeek{ is_eof: true };
            }
            if (_.isEolChar(_.src[cur])) {
                _.pos = _.consumeNewlineAt(cur);
                continue;
            }
            if (_.src[cur] == '#') {
                _.pos = cur;
                _.skipToNextLine();
                continue;
            }
            _.pos = line_start;
            let is_start = false;
            let is_end = false;
            if (spaces == 0) {
                is_start = _.isDocSepAt(cur, '-');
                is_end = _.isDocSepAt(cur, '.');
            }
            return LinePeek{
                is_eof: false,
                indent: spaces,
                content_pos: cur,
                is_doc_start: is_start,
                is_doc_end: is_end,
            };
        }
        return LinePeek{ is_eof: true };
    }

    fn isSeqItemAt(idx: usize): bool {
        if (idx >= _.src.len || _.src[idx] != '-') return false;
        if (_.isDocSepAt(idx, '-')) return false;
        if (idx + 1 >= _.src.len) return true;
        const next = _.src[idx + 1];
        return next == ' ' || next == '\r' || next == '\n' || next == '#';
    }

    /// Checks whether the current line from `start` contains an unquoted block mapping colon.
    fn hasBlockMappingColon(start: usize): bool {
        let i = start;
        let in_sq = false;
        let in_dq = false;
        let flow_depth: usize = 0;

        while (i < _.src.len && !_.isEolChar(_.src[i])) {
            const c = _.src[i];
            if (in_dq) {
                if (c == '\\' && i + 1 < _.src.len) {
                    i += 2;
                    continue;
                }
                if (c == '"') in_dq = false;
                i += 1;
                continue;
            }
            if (in_sq) {
                if (c == '\'' && i + 1 < _.src.len && _.src[i + 1] == '\'') {
                    i += 2;
                    continue;
                }
                if (c == '\'') in_sq = false;
                i += 1;
                continue;
            }
            if (c == '"') {
                in_dq = true;
                i += 1;
                continue;
            }
            if (c == '\'') {
                in_sq = true;
                i += 1;
                continue;
            }
            if (c == '[' || c == '{') {
                flow_depth += 1;
                i += 1;
                continue;
            }
            if ((c == ']' || c == '}') && flow_depth > 0) {
                flow_depth -= 1;
                i += 1;
                continue;
            }
            if (flow_depth == 0) {
                if (c == '#' && (i == start || _.isWsChar(_.src[i - 1]))) {
                    return false;
                }
                if (c == ':') {
                    if (i + 1 >= _.src.len) return true;
                    const nc = _.src[i + 1];
                    if (nc == ' ' || nc == '\t' || nc == '\r' || nc == '\n') {
                        return true;
                    }
                }
            }
            i += 1;
        }
        return false;
    }

    fn parseDoubleQuoted(*_): ([]const u8)! {
        if (_.pos >= _.src.len || _.src[_.pos] != '"') return YamlError.UnterminatedString;
        _.pos += 1;
        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(_.alloc);

        while (_.pos < _.src.len) {
            const hit = yamlSimdFindDqSpecial(_.src, _.pos);
            if (hit > _.pos) {
                buf.appendSlice(_.alloc, _.src[_.pos..hit]) catch return YamlError.OutOfMemory;
                _.pos = hit;
            }
            if (_.pos >= _.src.len) break;
            const c = _.src[_.pos];
            if (c == '"') {
                _.pos += 1;
                return buf.toOwnedSlice(_.alloc) catch return YamlError.OutOfMemory;
            }
            if (c == '\\') {
                _.pos += 1;
                if (_.pos >= _.src.len) return YamlError.UnterminatedString;
                const esc = _.src[_.pos];
                _.pos += 1;
                switch (esc) {
                    'n' => buf.append(_.alloc, '\n') catch return YamlError.OutOfMemory,
                    'r' => buf.append(_.alloc, '\r') catch return YamlError.OutOfMemory,
                    't' => buf.append(_.alloc, '\t') catch return YamlError.OutOfMemory,
                    '\\' => buf.append(_.alloc, '\\') catch return YamlError.OutOfMemory,
                    '"' => buf.append(_.alloc, '"') catch return YamlError.OutOfMemory,
                    '/' => buf.append(_.alloc, '/') catch return YamlError.OutOfMemory,
                    '0' => buf.append(_.alloc, 0) catch return YamlError.OutOfMemory,
                    'a' => buf.append(_.alloc, 0x07) catch return YamlError.OutOfMemory,
                    'b' => buf.append(_.alloc, 0x08) catch return YamlError.OutOfMemory,
                    'e' => buf.append(_.alloc, 0x1B) catch return YamlError.OutOfMemory,
                    'f' => buf.append(_.alloc, 0x0C) catch return YamlError.OutOfMemory,
                    'v' => buf.append(_.alloc, 0x0B) catch return YamlError.OutOfMemory,
                    ' ' => buf.append(_.alloc, ' ') catch return YamlError.OutOfMemory,
                    '\n' => {
                        while (_.pos < _.src.len && (_.src[_.pos] == ' ' || _.src[_.pos] == '\t')) {
                            _.pos += 1;
                        }
                    },
                    '\r' => {
                        if (_.pos < _.src.len && _.src[_.pos] == '\n') _.pos += 1;
                        while (_.pos < _.src.len && (_.src[_.pos] == ' ' || _.src[_.pos] == '\t')) {
                            _.pos += 1;
                        }
                    },
                    'x' => {
                        if (_.pos + 2 > _.src.len) return YamlError.UnterminatedString;
                        const cp = std.fmt.parseInt(u21, _.src[_.pos.._.pos + 2], 16) catch return YamlError.UnterminatedString;
                        _.pos += 2;
                        _.appendUtf8(&buf, cp)?;
                    },
                    'u' => {
                        if (_.pos + 4 > _.src.len) return YamlError.UnterminatedString;
                        const cp = std.fmt.parseInt(u21, _.src[_.pos.._.pos + 4], 16) catch return YamlError.UnterminatedString;
                        _.pos += 4;
                        _.appendUtf8(&buf, cp)?;
                    },
                    'U' => {
                        if (_.pos + 8 > _.src.len) return YamlError.UnterminatedString;
                        const cp = std.fmt.parseInt(u21, _.src[_.pos.._.pos + 8], 16) catch return YamlError.UnterminatedString;
                        _.pos += 8;
                        _.appendUtf8(&buf, cp)?;
                    },
                    else => buf.append(_.alloc, esc) catch return YamlError.OutOfMemory,
                }
                continue;
            }
            if (c == '\r') {
                _.pos += 1;
                if (_.pos < _.src.len && _.src[_.pos] == '\n') _.pos += 1;
                buf.append(_.alloc, '\n') catch return YamlError.OutOfMemory;
                continue;
            }
            buf.append(_.alloc, c) catch return YamlError.OutOfMemory;
            _.pos += 1;
        }
        return YamlError.UnterminatedString;
    }

    fn appendUtf8(*_, buf: *std.ArrayList(u8), cp: u21): ! {
        let tmp: [4]u8 = undefined;
        const len = std.unicode.utf8Encode(cp, &tmp) catch return YamlError.UnterminatedString;
        buf.appendSlice(_.alloc, tmp[0..len]) catch return YamlError.OutOfMemory;
    }

    fn parseSingleQuoted(*_): ([]const u8)! {
        if (_.pos >= _.src.len || _.src[_.pos] != '\'') return YamlError.UnterminatedString;
        _.pos += 1;
        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(_.alloc);

        while (_.pos < _.src.len) {
            const hit = yamlSimdFindSqSpecial(_.src, _.pos);
            if (hit > _.pos) {
                buf.appendSlice(_.alloc, _.src[_.pos..hit]) catch return YamlError.OutOfMemory;
                _.pos = hit;
            }
            if (_.pos >= _.src.len) break;
            const c = _.src[_.pos];
            if (c == '\'') {
                if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '\'') {
                    buf.append(_.alloc, '\'') catch return YamlError.OutOfMemory;
                    _.pos += 2;
                    continue;
                }
                _.pos += 1;
                return buf.toOwnedSlice(_.alloc) catch return YamlError.OutOfMemory;
            }
            if (c == '\r') {
                _.pos += 1;
                if (_.pos < _.src.len && _.src[_.pos] == '\n') _.pos += 1;
                buf.append(_.alloc, '\n') catch return YamlError.OutOfMemory;
                continue;
            }
            buf.append(_.alloc, c) catch return YamlError.OutOfMemory;
            _.pos += 1;
        }
        return YamlError.UnterminatedString;
    }

    fn trimRightWs(s: []const u8): []const u8 {
        let end = s.len;
        while (end > 0 && (s[end - 1] == ' ' || s[end - 1] == '\t')) {
            end -= 1;
        }
        return s[0..end];
    }

    fn stripInlineComment(raw: []const u8): []const u8 {
        let i: usize = 0;
        while (i < raw.len) {
            if (raw[i] == '#' && (i == 0 || _.isWsChar(raw[i - 1]))) {
                return _.trimRightWs(raw[0..i]);
            }
            i += 1;
        }
        return _.trimRightWs(raw);
    }

    fn inferPlainScalar(raw_in: []const u8): YamlValue! {
        const s = _.stripInlineComment(raw_in);
        if (s.len == 0 ||
            std.mem.eql(u8, s, "null") ||
            std.mem.eql(u8, s, "Null") ||
            std.mem.eql(u8, s, "NULL") ||
            std.mem.eql(u8, s, "~"))
        {
            return YamlValue{ kind: .Null };
        }
        if (std.mem.eql(u8, s, "true") ||
            std.mem.eql(u8, s, "True") ||
            std.mem.eql(u8, s, "TRUE"))
        {
            return YamlValue{ kind: .Bool, bool_val: true };
        }
        if (std.mem.eql(u8, s, "false") ||
            std.mem.eql(u8, s, "False") ||
            std.mem.eql(u8, s, "FALSE"))
        {
            return YamlValue{ kind: .Bool, bool_val: false };
        }

        if (iv := YamlParser.tryParseInt(s)) {
            return YamlValue{ kind: .Int, int_val: iv };
        }
        if (fv := YamlParser.tryParseFloat(s)) {
            return YamlValue{ kind: .Float, float_val: fv };
        }

        const dup = _.dupStr(s)?;
        return YamlValue{ kind: .String, str_val: dup };
    }

    static fn tryParseInt(s: []const u8): i64? {
        if (s.len == 0) return null;
        if (std.mem.startsWith(u8, s, "0x") || std.mem.startsWith(u8, s, "0X")) {
            if (s.len == 2) return null;
            return std.fmt.parseInt(i64, s[2..], 16) catch return null;
        }
        if (std.mem.startsWith(u8, s, "0o") || std.mem.startsWith(u8, s, "0O")) {
            if (s.len == 2) return null;
            return std.fmt.parseInt(i64, s[2..], 8) catch return null;
        }
        let i: usize = 0;
        if (s[0] == '+' || s[0] == '-') {
            if (s.len == 1) return null;
            i = 1;
        }
        while (i < s.len) {
            if (s[i] < '0' || s[i] > '9') return null;
            i += 1;
        }
        return std.fmt.parseInt(i64, s, 10) catch return null;
    }

    static fn tryParseFloat(s: []const u8): f64? {
        if (s.len == 0) return null;
        if (std.mem.eql(u8, s, ".inf") || std.mem.eql(u8, s, ".Inf") || std.mem.eql(u8, s, ".INF") ||
            std.mem.eql(u8, s, "+.inf") || std.mem.eql(u8, s, "+.Inf") || std.mem.eql(u8, s, "+.INF"))
        {
            return std.math.inf(f64);
        }
        if (std.mem.eql(u8, s, "-.inf") || std.mem.eql(u8, s, "-.Inf") || std.mem.eql(u8, s, "-.INF")) {
            return -std.math.inf(f64);
        }
        if (std.mem.eql(u8, s, ".nan") || std.mem.eql(u8, s, ".NaN") || std.mem.eql(u8, s, ".NAN")) {
            return std.math.nan(f64);
        }
        let has_dot_or_exp = false;
        let has_digit = false;
        let i: usize = 0;
        if (s[0] == '+' || s[0] == '-') i = 1;
        while (i < s.len) {
            const c = s[i];
            if (c >= '0' && c <= '9') {
                has_digit = true;
            } else if (c == '.' || c == 'e' || c == 'E') {
                has_dot_or_exp = true;
            } else if ((c == '+' || c == '-') && i > 0 && (s[i - 1] == 'e' || s[i - 1] == 'E')) {
                // exponent sign
            } else {
                return null;
            }
            i += 1;
        }
        if (!has_digit || !has_dot_or_exp) return null;
        return std.fmt.parseFloat(f64, s) catch return null;
    }

    /// Parses literal (`|` / `|-` / `|+`) or folded (`>` / `>-` / `>+`) block scalars.
    fn parseBlockScalar(*_, parent_indent: usize): YamlValue! {
        const indicator = _.src[_.pos];
        const is_folded = indicator == '>';
        _.pos += 1;

        let chomp: u8 = 'c'; // 'c' = clip, 's' = strip (-), 'k' = keep (+)
        let explicit_indent: usize = 0;

        while (_.pos < _.src.len && !_.isEolChar(_.src[_.pos])) {
            const c = _.src[_.pos];
            if (c == '-') {
                chomp = 's';
                _.pos += 1;
            } else if (c == '+') {
                chomp = 'k';
                _.pos += 1;
            } else if (c >= '1' && c <= '9') {
                explicit_indent = (c - '0') as usize;
                _.pos += 1;
            } else if (c == ' ' || c == '\t' || c == '#') {
                break;
            } else {
                break;
            }
        }
        _.skipToNextLine();

        let content_indent: usize = if (explicit_indent > 0) parent_indent + explicit_indent else 0;
        let lines: std.ArrayList(string) = .empty;
        defer lines.deinit(_.alloc);

        while (_.pos < _.src.len) {
            const line_start = _.pos;
            const cur = yamlSimdSkipSpaces(_.src, line_start);
            const spaces: usize = cur - line_start;
            if (cur < _.src.len && _.src[cur] == '\t') {
                if (content_indent == 0 || spaces < content_indent) {
                    return YamlError.TabIndentationForbidden;
                }
            }
            if (cur >= _.src.len || _.isEolChar(_.src[cur])) {
                // Blank line
                if (content_indent > 0 && spaces > content_indent) {
                    lines.append(_.alloc, _.src[line_start + content_indent..cur]) catch return YamlError.OutOfMemory;
                } else {
                    lines.append(_.alloc, "") catch return YamlError.OutOfMemory;
                }
                _.pos = _.consumeNewlineAt(cur);
                continue;
            }
            if (content_indent == 0) {
                if (spaces <= parent_indent) {
                    _.pos = line_start;
                    break;
                }
                content_indent = spaces;
            } else if (spaces < content_indent) {
                _.pos = line_start;
                break;
            }

            const eol = yamlSimdFindEol(_.src, cur);
            lines.append(_.alloc, _.src[line_start + content_indent..eol]) catch return YamlError.OutOfMemory;
            _.pos = _.consumeNewlineAt(eol);
        }

        // Trim trailing empty lines for chomping calculation
        let last_non_empty: usize = lines.items.len;
        while (last_non_empty > 0 && lines.items[last_non_empty - 1].len == 0) {
            last_non_empty -= 1;
        }
        if (last_non_empty == 0) {
            if (chomp == 'k') {
                let empty_buf: std.ArrayList(u8) = .empty;
                errdefer empty_buf.deinit(_.alloc);
                let li: usize = 0;
                while (li < lines.items.len) {
                    empty_buf.append(_.alloc, '\n') catch return YamlError.OutOfMemory;
                    li += 1;
                }
                const out_k = empty_buf.toOwnedSlice(_.alloc) catch return YamlError.OutOfMemory;
                return YamlValue{ kind: .String, str_val: out_k };
            }
            return YamlValue{ kind: .String, str_val: "" };
        }

        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(_.alloc);

        if (!is_folded) {
            let idx: usize = 0;
            while (idx < last_non_empty) {
                if (idx > 0) buf.append(_.alloc, '\n') catch return YamlError.OutOfMemory;
                buf.appendSlice(_.alloc, lines.items[idx]) catch return YamlError.OutOfMemory;
                idx += 1;
            }
        } else {
            let idx: usize = 0;
            let prev_was_empty = false;
            let prev_was_indented = false;
            while (idx < last_non_empty) {
                const ln = lines.items[idx];
                const is_empty = ln.len == 0;
                const is_indented = !is_empty && (ln[0] == ' ' || ln[0] == '\t');
                if (idx > 0) {
                    if (is_empty) {
                        buf.append(_.alloc, '\n') catch return YamlError.OutOfMemory;
                    } else if (prev_was_empty) {
                        if (prev_was_indented) buf.append(_.alloc, '\n') catch return YamlError.OutOfMemory;
                    } else if (prev_was_indented || is_indented) {
                        buf.append(_.alloc, '\n') catch return YamlError.OutOfMemory;
                    } else {
                        buf.append(_.alloc, ' ') catch return YamlError.OutOfMemory;
                    }
                }
                if (!is_empty) {
                    buf.appendSlice(_.alloc, ln) catch return YamlError.OutOfMemory;
                    prev_was_indented = is_indented;
                }
                prev_was_empty = is_empty;
                idx += 1;
            }
        }

        if (chomp == 'c') {
            buf.append(_.alloc, '\n') catch return YamlError.OutOfMemory;
        } else if (chomp == 'k') {
            let k_idx = last_non_empty;
            buf.append(_.alloc, '\n') catch return YamlError.OutOfMemory;
            while (k_idx < lines.items.len) {
                buf.append(_.alloc, '\n') catch return YamlError.OutOfMemory;
                k_idx += 1;
            }
        }

        const out_str = buf.toOwnedSlice(_.alloc) catch return YamlError.OutOfMemory;
        return YamlValue{ kind: .String, str_val: out_str };
    }

    fn skipFlowWsAndComments(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                _.pos += 1;
                continue;
            }
            if (c == '#') {
                _.skipToNextLine();
                continue;
            }
            break;
        }
    }

    fn readTokenName(*_): []const u8 {
        const start = _.pos;
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ',' || c == ']' || c == '}' || c == ':') {
                break;
            }
            _.pos += 1;
        }
        return _.src[start.._.pos];
    }

    fn parseFlowValue(*_, depth: usize): YamlValue! {
        if (depth >= MAX_YAML_DEPTH) return YamlError.MaxDepthExceeded;
        _.skipFlowWsAndComments();
        if (_.pos >= _.src.len) return YamlError.UnterminatedFlow;

        let anchor_name: []const u8 = "";
        if (_.src[_.pos] == '&') {
            _.pos += 1;
            anchor_name = _.readTokenName();
            _.skipFlowWsAndComments();
            if (_.pos >= _.src.len) return YamlError.UnterminatedFlow;
        }

        let val: YamlValue = .{};
        const c = _.src[_.pos];
        if (c == '*') {
            _.pos += 1;
            const alias_name = _.readTokenName();
            if (alias_name.len == 0) return YamlError.InvalidAlias;
            val = _.resolveAlias(alias_name)?;
        } else if (c == '[') {
            val = _.parseFlowSeq(depth)?;
        } else if (c == '{') {
            val = _.parseFlowMap(depth)?;
        } else if (c == '"') {
            const s = _.parseDoubleQuoted()?;
            val = YamlValue{ kind: .String, str_val: s };
        } else if (c == '\'') {
            const s = _.parseSingleQuoted()?;
            val = YamlValue{ kind: .String, str_val: s };
        } else {
            const start = _.pos;
            while (_.pos < _.src.len) {
                const sc = _.src[_.pos];
                if (sc == ',' || sc == ']' || sc == '}' || sc == '\r' || sc == '\n') break;
                if (sc == '#' && _.pos > start && _.isWsChar(_.src[_.pos - 1])) break;
                _.pos += 1;
            }
            val = _.inferPlainScalar(_.src[start.._.pos])?;
        }

        if (anchor_name.len > 0) {
            errdefer val.deinit(_.alloc);
            _.registerAnchor(anchor_name, val)?;
        }
        return val;
    }

    fn parseFlowSeq(*_, depth: usize): YamlValue! {
        if (depth >= MAX_YAML_DEPTH) return YamlError.MaxDepthExceeded;
        _.pos += 1; // skip '['
        let items: std.ArrayList(YamlValue) = .empty;
        errdefer {
            for (*it of items.items) it.deinit(_.alloc);
            items.deinit(_.alloc);
        }

        while (true) {
            _.skipFlowWsAndComments();
            if (_.pos >= _.src.len) return YamlError.UnterminatedFlow;
            if (_.src[_.pos] == ']') {
                _.pos += 1;
                const slice = items.toOwnedSlice(_.alloc) catch return YamlError.OutOfMemory;
                return YamlValue{ kind: .Sequence, seq_val: slice };
            }
            let elem = _.parseFlowValue(depth + 1)?;
            errdefer elem.deinit(_.alloc);
            items.append(_.alloc, elem) catch return YamlError.OutOfMemory;

            _.skipFlowWsAndComments();
            if (_.pos >= _.src.len) return YamlError.UnterminatedFlow;
            if (_.src[_.pos] == ',') {
                _.pos += 1;
                continue;
            }
            if (_.src[_.pos] == ']') {
                _.pos += 1;
                const slice = items.toOwnedSlice(_.alloc) catch return YamlError.OutOfMemory;
                return YamlValue{ kind: .Sequence, seq_val: slice };
            }
            return YamlError.UnterminatedFlow;
        }
    }

    fn parseFlowMap(*_, depth: usize): YamlValue! {
        if (depth >= MAX_YAML_DEPTH) return YamlError.MaxDepthExceeded;
        _.pos += 1; // skip '{'
        let entries: std.ArrayList(YamlEntry) = .empty;
        errdefer {
            for (*e of entries.items) {
                if (e.key.len > 0) _.alloc.free(e.key);
                e.value.deinit(_.alloc);
            }
            entries.deinit(_.alloc);
        }

        while (true) {
            _.skipFlowWsAndComments();
            if (_.pos >= _.src.len) return YamlError.UnterminatedFlow;
            if (_.src[_.pos] == '}') {
                _.pos += 1;
                const slice = entries.toOwnedSlice(_.alloc) catch return YamlError.OutOfMemory;
                return YamlValue{ kind: .Mapping, map_val: slice };
            }

            let key: []const u8 = "";
            if (_.src[_.pos] == '"') {
                key = _.parseDoubleQuoted()?;
            } else if (_.src[_.pos] == '\'') {
                key = _.parseSingleQuoted()?;
            } else {
                const kstart = _.pos;
                while (_.pos < _.src.len) {
                    const kc = _.src[_.pos];
                    if (kc == ':' || kc == ',' || kc == '}' || kc == '\r' || kc == '\n') break;
                    _.pos += 1;
                }
                key = _.dupStr(_.trimRightWs(_.src[kstart.._.pos]))?;
            }
            errdefer {
                if (key.len > 0) _.alloc.free(key);
            }

            for (existing of entries.items) {
                if (std.mem.eql(u8, existing.key, key)) {
                    return YamlError.DuplicateKey;
                }
            }

            _.skipFlowWsAndComments();
            if (_.pos >= _.src.len || _.src[_.pos] != ':') return YamlError.UnterminatedFlow;
            _.pos += 1;

            _.skipFlowWsAndComments();
            let val: YamlValue = YamlValue{ kind: .Null };
            if (_.pos < _.src.len && _.src[_.pos] != ',' && _.src[_.pos] != '}') {
                val = _.parseFlowValue(depth + 1)?;
            }
            errdefer val.deinit(_.alloc);

            entries.append(_.alloc, YamlEntry{ key: key, value: val }) catch return YamlError.OutOfMemory;

            _.skipFlowWsAndComments();
            if (_.pos >= _.src.len) return YamlError.UnterminatedFlow;
            if (_.src[_.pos] == ',') {
                _.pos += 1;
                continue;
            }
            if (_.src[_.pos] == '}') {
                _.pos += 1;
                const slice = entries.toOwnedSlice(_.alloc) catch return YamlError.OutOfMemory;
                return YamlValue{ kind: .Mapping, map_val: slice };
            }
            return YamlError.UnterminatedFlow;
        }
    }

    fn parseInlineOrBlockScalar(*_, parent_indent: usize, depth: usize): YamlValue! {
        if (depth >= MAX_YAML_DEPTH) return YamlError.MaxDepthExceeded;
        _.skipHorizWs();

        let anchor_name: []const u8 = "";
        if (_.pos < _.src.len && _.src[_.pos] == '&') {
            _.pos += 1;
            anchor_name = _.readTokenName();
            _.skipHorizWs();
            if (_.pos >= _.src.len || _.isEolChar(_.src[_.pos]) || _.src[_.pos] == '#') {
                _.skipToNextLine();
                let block_val = _.parseBlockNode(parent_indent + 1, depth)?;
                if (anchor_name.len > 0) {
                    errdefer block_val.deinit(_.alloc);
                    _.registerAnchor(anchor_name, block_val)?;
                }
                return block_val;
            }
        }

        let val: YamlValue = .{};
        if (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == '*') {
                _.pos += 1;
                const alias_name = _.readTokenName();
                if (alias_name.len == 0) return YamlError.InvalidAlias;
                _.skipToNextLine();
                val = _.resolveAlias(alias_name)?;
            } else if (c == '|' || c == '>') {
                val = _.parseBlockScalar(parent_indent)?;
            } else if (c == '[') {
                val = _.parseFlowSeq(depth)?;
                _.skipToNextLine();
            } else if (c == '{') {
                val = _.parseFlowMap(depth)?;
                _.skipToNextLine();
            } else if (c == '"') {
                const s = _.parseDoubleQuoted()?;
                _.skipToNextLine();
                val = YamlValue{ kind: .String, str_val: s };
            } else if (c == '\'') {
                const s = _.parseSingleQuoted()?;
                _.skipToNextLine();
                val = YamlValue{ kind: .String, str_val: s };
            } else {
                const start = _.pos;
                _.pos = yamlSimdFindEol(_.src, _.pos);
                const raw = _.src[start.._.pos];
                _.pos = _.consumeNewlineAt(_.pos);
                val = _.inferPlainScalar(raw)?;
            }
        }

        if (anchor_name.len > 0) {
            errdefer val.deinit(_.alloc);
            _.registerAnchor(anchor_name, val)?;
        }
        return val;
    }

    fn parseBlockKey(*_): ([]const u8)! {
        if (_.pos >= _.src.len) return YamlError.UnexpectedEof;
        if (_.src[_.pos] == '"') {
            const k = _.parseDoubleQuoted()?;
            _.skipHorizWs();
            if (_.pos >= _.src.len || _.src[_.pos] != ':') return YamlError.InvalidIndentation;
            return k;
        }
        if (_.src[_.pos] == '\'') {
            const k = _.parseSingleQuoted()?;
            _.skipHorizWs();
            if (_.pos >= _.src.len || _.src[_.pos] != ':') return YamlError.InvalidIndentation;
            return k;
        }
        const start = _.pos;
        while (_.pos < _.src.len && !_.isEolChar(_.src[_.pos])) {
            if (_.src[_.pos] == ':') {
                if (_.pos + 1 >= _.src.len) break;
                const nc = _.src[_.pos + 1];
                if (nc == ' ' || nc == '\t' || nc == '\r' || nc == '\n') break;
            }
            _.pos += 1;
        }
        if (_.pos >= _.src.len || _.src[_.pos] != ':') return YamlError.InvalidIndentation;
        return _.dupStr(_.trimRightWs(_.src[start.._.pos]));
    }

    fn parseMapValueAfterColon(*_, map_indent: usize, depth: usize): YamlValue! {
        _.pos += 1; // skip ':'
        _.skipHorizWs();
        if (_.pos < _.src.len && !_.isEolChar(_.src[_.pos]) && _.src[_.pos] != '#') {
            return _.parseInlineOrBlockScalar(map_indent, depth + 1);
        }
        _.skipToNextLine();
        const next_lp = _.peekNextLine()?;
        if (next_lp.is_eof || next_lp.is_doc_start || next_lp.is_doc_end) {
            return YamlValue{ kind: .Null };
        }
        if (next_lp.indent > map_indent) {
            return _.parseBlockNode(map_indent + 1, depth + 1);
        }
        if (next_lp.indent == map_indent && _.isSeqItemAt(next_lp.content_pos)) {
            return _.parseBlockSeq(map_indent, depth + 1);
        }
        return YamlValue{ kind: .Null };
    }

    fn parseBlockMap(*_, map_indent: usize, depth: usize): YamlValue! {
        if (depth >= MAX_YAML_DEPTH) return YamlError.MaxDepthExceeded;
        let entries: std.ArrayList(YamlEntry) = .empty;
        errdefer {
            for (*e of entries.items) {
                if (e.key.len > 0) _.alloc.free(e.key);
                e.value.deinit(_.alloc);
            }
            entries.deinit(_.alloc);
        }

        while (true) {
            const lp = _.peekNextLine()?;
            if (lp.is_eof || lp.is_doc_start || lp.is_doc_end) break;
            if (lp.indent != map_indent) break;
            if (_.isSeqItemAt(lp.content_pos)) break;
            if (!_.hasBlockMappingColon(lp.content_pos)) break;

            _.pos = lp.content_pos;
            const key = _.parseBlockKey()?;
            errdefer {
                if (key.len > 0) _.alloc.free(key);
            }
            for (existing of entries.items) {
                if (std.mem.eql(u8, existing.key, key)) {
                    return YamlError.DuplicateKey;
                }
            }
            let val = _.parseMapValueAfterColon(map_indent, depth)?;
            errdefer val.deinit(_.alloc);
            entries.append(_.alloc, YamlEntry{ key: key, value: val }) catch return YamlError.OutOfMemory;
        }

        const slice = entries.toOwnedSlice(_.alloc) catch return YamlError.OutOfMemory;
        return YamlValue{ kind: .Mapping, map_val: slice };
    }

    fn parseBlockSeq(*_, seq_indent: usize, depth: usize): YamlValue! {
        if (depth >= MAX_YAML_DEPTH) return YamlError.MaxDepthExceeded;
        let items: std.ArrayList(YamlValue) = .empty;
        errdefer {
            for (*it of items.items) it.deinit(_.alloc);
            items.deinit(_.alloc);
        }

        while (true) {
            const lp = _.peekNextLine()?;
            if (lp.is_eof || lp.is_doc_start || lp.is_doc_end) break;
            if (lp.indent != seq_indent) break;
            if (!_.isSeqItemAt(lp.content_pos)) break;

            _.pos = lp.content_pos + 1; // skip '-'
            let after_dash_spaces: usize = 0;
            while (_.pos < _.src.len && _.src[_.pos] == ' ') {
                after_dash_spaces += 1;
                _.pos += 1;
            }
            if (_.pos < _.src.len && _.src[_.pos] == '\t') {
                return YamlError.TabIndentationForbidden;
            }

            let elem: YamlValue = YamlValue{ kind: .Null };
            if (_.pos >= _.src.len || _.isEolChar(_.src[_.pos]) || _.src[_.pos] == '#') {
                _.skipToNextLine();
                const child_lp = _.peekNextLine()?;
                if (!child_lp.is_eof && !child_lp.is_doc_start && !child_lp.is_doc_end && child_lp.indent > seq_indent) {
                    elem = _.parseBlockNode(seq_indent + 1, depth + 1)?;
                }
            } else if (_.hasBlockMappingColon(_.pos)) {
                const nested_indent = seq_indent + 1 + after_dash_spaces;
                elem = _.parseInlineSeqMapping(nested_indent, depth + 1)?;
            } else {
                elem = _.parseInlineOrBlockScalar(seq_indent, depth + 1)?;
            }

            errdefer elem.deinit(_.alloc);
            items.append(_.alloc, elem) catch return YamlError.OutOfMemory;
        }

        const slice = items.toOwnedSlice(_.alloc) catch return YamlError.OutOfMemory;
        return YamlValue{ kind: .Sequence, seq_val: slice };
    }

    /// Parses `- key: val` where the first key-value pair sits on the same line as `-`.
    fn parseInlineSeqMapping(*_, map_indent: usize, depth: usize): YamlValue! {
        if (depth >= MAX_YAML_DEPTH) return YamlError.MaxDepthExceeded;
        let entries: std.ArrayList(YamlEntry) = .empty;
        errdefer {
            for (*e of entries.items) {
                if (e.key.len > 0) _.alloc.free(e.key);
                e.value.deinit(_.alloc);
            }
            entries.deinit(_.alloc);
        }

        // First entry on the `- ` line
        const first_key = _.parseBlockKey()?;
        errdefer {
            if (first_key.len > 0) _.alloc.free(first_key);
        }
        let first_val = _.parseMapValueAfterColon(map_indent, depth)?;
        errdefer first_val.deinit(_.alloc);
        entries.append(_.alloc, YamlEntry{ key: first_key, value: first_val }) catch return YamlError.OutOfMemory;

        // Subsequent sibling entries at `map_indent`
        while (true) {
            const lp = _.peekNextLine()?;
            if (lp.is_eof || lp.is_doc_start || lp.is_doc_end) break;
            if (lp.indent != map_indent) break;
            if (_.isSeqItemAt(lp.content_pos)) break;
            if (!_.hasBlockMappingColon(lp.content_pos)) break;

            _.pos = lp.content_pos;
            const key = _.parseBlockKey()?;
            errdefer {
                if (key.len > 0) _.alloc.free(key);
            }
            for (existing of entries.items) {
                if (std.mem.eql(u8, existing.key, key)) {
                    return YamlError.DuplicateKey;
                }
            }
            let val = _.parseMapValueAfterColon(map_indent, depth)?;
            errdefer val.deinit(_.alloc);
            entries.append(_.alloc, YamlEntry{ key: key, value: val }) catch return YamlError.OutOfMemory;
        }

        const slice = entries.toOwnedSlice(_.alloc) catch return YamlError.OutOfMemory;
        return YamlValue{ kind: .Mapping, map_val: slice };
    }

    fn parseBlockNode(*_, min_indent: usize, depth: usize): YamlValue! {
        if (depth >= MAX_YAML_DEPTH) return YamlError.MaxDepthExceeded;
        const lp = _.peekNextLine()?;
        if (lp.is_eof || lp.is_doc_start || lp.is_doc_end) {
            return YamlValue{ kind: .Null };
        }
        if (lp.indent < min_indent) {
            return YamlValue{ kind: .Null };
        }

        _.pos = lp.content_pos;
        if (_.isSeqItemAt(_.pos)) {
            return _.parseBlockSeq(lp.indent, depth);
        }
        if (_.hasBlockMappingColon(_.pos)) {
            return _.parseBlockMap(lp.indent, depth);
        }
        return _.parseInlineOrBlockScalar(lp.indent, depth);
    }

    pub static fn parse(alloc: std.mem.Allocator, input: []const u8): YamlDoc! {
        let parser = YamlParser.init(alloc, input);
        defer parser.deinit();

        let doc = YamlDoc.init(alloc);
        errdefer doc.deinit();

        // Skip optional leading `---` markers
        while (true) {
            const lp = parser.peekNextLine()?;
            if (lp.is_eof) {
                return doc;
            }
            if (lp.is_doc_start) {
                parser.pos = lp.content_pos + 3;
                parser.skipToNextLine();
                continue;
            }
            if (lp.is_doc_end) {
                parser.pos = lp.content_pos + 3;
                parser.skipToNextLine();
                return doc;
            }
            break;
        }

        doc.root = parser.parseBlockNode(0, 0)?;

        // Validate remaining trailing lines do not have tab indentation
        while (true) {
            const tail = parser.peekNextLine()?;
            if (tail.is_eof || tail.is_doc_start || tail.is_doc_end) break;
            parser.pos = tail.content_pos;
            parser.skipToNextLine();
        }
        return doc;
    }

    pub static fn parseAll(alloc: std.mem.Allocator, input: []const u8): YamlDoc! {
        let parser = YamlParser.init(alloc, input);
        defer parser.deinit();

        let docs: std.ArrayList(YamlValue) = .empty;
        errdefer {
            for (*d of docs.items) d.deinit(alloc);
            docs.deinit(alloc);
        }

        while (true) {
            const lp = parser.peekNextLine()?;
            if (lp.is_eof) break;
            if (lp.is_doc_start) {
                parser.pos = lp.content_pos + 3;
                parser.skipToNextLine();
                continue;
            }
            if (lp.is_doc_end) {
                parser.pos = lp.content_pos + 3;
                parser.skipToNextLine();
                continue;
            }
            let val = parser.parseBlockNode(0, 0)?;
            errdefer val.deinit(alloc);
            docs.append(alloc, val) catch return YamlError.OutOfMemory;
        }

        let doc = YamlDoc.init(alloc);
        if (docs.items.len > 0) {
            doc.documents = docs.toOwnedSlice(alloc) catch return YamlError.OutOfMemory;
            doc.root = doc.documents[0];
        }
        return doc;
    }
}

pub static class YamlSerializer {
    pub fn stringify(alloc: std.mem.Allocator, doc: YamlDoc): ([]const u8)! {
        return YamlSerializer.stringifyValue(alloc, doc.root);
    }

    pub fn stringifyValue(alloc: std.mem.Allocator, val: YamlValue): ([]const u8)! {
        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(alloc);
        YamlSerializer.writeYamlNode(alloc, &buf, val, 0)?;
        if (buf.items.len == 0 || buf.items[buf.items.len - 1] != '\n') {
            buf.append(alloc, '\n') catch return YamlError.OutOfMemory;
        }
        return buf.toOwnedSlice(alloc) catch return YamlError.OutOfMemory;
    }

    pub fn toJson(alloc: std.mem.Allocator, doc: YamlDoc): ([]const u8)! {
        return YamlSerializer.valueToJson(alloc, doc.root);
    }

    pub fn valueToJson(alloc: std.mem.Allocator, val: YamlValue): ([]const u8)! {
        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(alloc);
        YamlSerializer.writeJsonNode(alloc, &buf, val)?;
        return buf.toOwnedSlice(alloc) catch return YamlError.OutOfMemory;
    }

    fn writeIndent(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), spaces: usize): ! {
        let i: usize = 0;
        while (i < spaces) {
            buf.append(alloc, ' ') catch return YamlError.OutOfMemory;
            i += 1;
        }
    }

    fn needsYamlQuotes(s: []const u8): bool {
        if (s.len == 0) return true;
        if (s[0] == ' ' || s[0] == '\t' || s[s.len - 1] == ' ' || s[s.len - 1] == '\t') return true;
        if (s[0] == '-' || s[0] == '?' || s[0] == ':' || s[0] == '[' || s[0] == ']' ||
            s[0] == '{' || s[0] == '}' || s[0] == '#' || s[0] == '&' || s[0] == '*' ||
            s[0] == '!' || s[0] == '|' || s[0] == '>' || s[0] == '\'' || s[0] == '"' ||
            s[0] == '%' || s[0] == '@' || s[0] == '`')
        {
            return true;
        }
        if (std.mem.eql(u8, s, "null") || std.mem.eql(u8, s, "Null") || std.mem.eql(u8, s, "NULL") ||
            std.mem.eql(u8, s, "~") || std.mem.eql(u8, s, "true") || std.mem.eql(u8, s, "True") ||
            std.mem.eql(u8, s, "TRUE") || std.mem.eql(u8, s, "false") || std.mem.eql(u8, s, "False") ||
            std.mem.eql(u8, s, "FALSE"))
        {
            return true;
        }
        if (YamlParser.tryParseInt(s) != null || YamlParser.tryParseFloat(s) != null) return true;
        let i: usize = 0;
        while (i < s.len) {
            const c = s[i];
            if (c < 0x20 || c == 0x7F || c == '"' || c == '\\') return true;
            if (c == ':' && (i + 1 >= s.len || s[i + 1] == ' ')) return true;
            if (c == '#' && i > 0 && s[i - 1] == ' ') return true;
            i += 1;
        }
        return false;
    }

    fn writeQuotedString(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), s: []const u8): ! {
        buf.append(alloc, '"') catch return YamlError.OutOfMemory;
        let i: usize = 0;
        while (i < s.len) {
            const hit = yamlSimdFindQuoteEsc(s, i);
            if (hit > i) {
                buf.appendSlice(alloc, s[i..hit]) catch return YamlError.OutOfMemory;
                i = hit;
            }
            if (i >= s.len) break;
            const c = s[i];
            switch (c) {
                '"' => buf.appendSlice(alloc, "\\\"") catch return YamlError.OutOfMemory,
                '\\' => buf.appendSlice(alloc, "\\\\") catch return YamlError.OutOfMemory,
                '\n' => buf.appendSlice(alloc, "\\n") catch return YamlError.OutOfMemory,
                '\r' => buf.appendSlice(alloc, "\\r") catch return YamlError.OutOfMemory,
                '\t' => buf.appendSlice(alloc, "\\t") catch return YamlError.OutOfMemory,
                else => {
                    const hex = "0123456789ABCDEF";
                    let esc: [6]u8 = ['\\', 'u', '0', '0', hex[(c >> 4) & 0xF], hex[c & 0xF]];
                    buf.appendSlice(alloc, &esc) catch return YamlError.OutOfMemory;
                },
            }
            i += 1;
        }
        buf.append(alloc, '"') catch return YamlError.OutOfMemory;
    }

    fn writeYamlScalar(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), val: YamlValue): ! {
        switch (val.kind) {
            .Null => buf.appendSlice(alloc, "null") catch return YamlError.OutOfMemory,
            .Bool => {
                if (val.bool_val) {
                    buf.appendSlice(alloc, "true") catch return YamlError.OutOfMemory;
                } else {
                    buf.appendSlice(alloc, "false") catch return YamlError.OutOfMemory;
                }
            },
            .Int => {
                let num_buf: [32]u8 = undefined;
                const num_str = std.fmt.bufPrint(&num_buf, "{d}", .{val.int_val}) catch return YamlError.OutOfMemory;
                buf.appendSlice(alloc, num_str) catch return YamlError.OutOfMemory;
            },
            .Float => {
                if (std.math.isNan(val.float_val)) {
                    buf.appendSlice(alloc, ".nan") catch return YamlError.OutOfMemory;
                } else if (std.math.isInf(val.float_val)) {
                    if (val.float_val < 0) {
                        buf.appendSlice(alloc, "-.inf") catch return YamlError.OutOfMemory;
                    } else {
                        buf.appendSlice(alloc, ".inf") catch return YamlError.OutOfMemory;
                    }
                } else {
                    let f_buf: [64]u8 = undefined;
                    const f_str = std.fmt.bufPrint(&f_buf, "{d}", .{val.float_val}) catch return YamlError.OutOfMemory;
                    buf.appendSlice(alloc, f_str) catch return YamlError.OutOfMemory;
                    if (std.mem.indexOfScalar(u8, f_str, '.') == null && std.mem.indexOfScalar(u8, f_str, 'e') == null) {
                        buf.appendSlice(alloc, ".0") catch return YamlError.OutOfMemory;
                    }
                }
            },
            .String => {
                if (YamlSerializer.needsYamlQuotes(val.str_val)) {
                    YamlSerializer.writeQuotedString(alloc, buf, val.str_val)?;
                } else {
                    buf.appendSlice(alloc, val.str_val) catch return YamlError.OutOfMemory;
                }
            },
            .Sequence => buf.appendSlice(alloc, "[]") catch return YamlError.OutOfMemory,
            .Mapping => buf.appendSlice(alloc, "{}") catch return YamlError.OutOfMemory,
        }
    }

    fn writeYamlKey(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), key: []const u8): ! {
        if (YamlSerializer.needsYamlQuotes(key)) {
            YamlSerializer.writeQuotedString(alloc, buf, key)?;
        } else {
            buf.appendSlice(alloc, key) catch return YamlError.OutOfMemory;
        }
    }

    fn writeYamlNode(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), val: YamlValue, indent: usize): ! {
        switch (val.kind) {
            .Sequence => {
                if (val.seq_val.len == 0) {
                    buf.appendSlice(alloc, "[]") catch return YamlError.OutOfMemory;
                    return;
                }
                let idx: usize = 0;
                for (item of val.seq_val) {
                    if (idx > 0) buf.append(alloc, '\n') catch return YamlError.OutOfMemory;
                    YamlSerializer.writeIndent(alloc, buf, indent)?;
                    buf.appendSlice(alloc, "- ") catch return YamlError.OutOfMemory;
                    if (item.kind == .Sequence && item.seq_val.len > 0) {
                        buf.append(alloc, '\n') catch return YamlError.OutOfMemory;
                        YamlSerializer.writeYamlNode(alloc, buf, item, indent + 2)?;
                    } else if (item.kind == .Mapping && item.map_val.len > 0) {
                        YamlSerializer.writeYamlMappingInlineFirst(alloc, buf, item.map_val, indent + 2)?;
                    } else {
                        YamlSerializer.writeYamlScalar(alloc, buf, item)?;
                    }
                    idx += 1;
                }
            },
            .Mapping => {
                if (val.map_val.len == 0) {
                    buf.appendSlice(alloc, "{}") catch return YamlError.OutOfMemory;
                    return;
                }
                let idx: usize = 0;
                for (entry of val.map_val) {
                    if (idx > 0) buf.append(alloc, '\n') catch return YamlError.OutOfMemory;
                    YamlSerializer.writeIndent(alloc, buf, indent)?;
                    YamlSerializer.writeYamlKey(alloc, buf, entry.key)?;
                    if ((entry.value.kind == .Mapping && entry.value.map_val.len > 0) ||
                        (entry.value.kind == .Sequence && entry.value.seq_val.len > 0))
                    {
                        buf.appendSlice(alloc, ":\n") catch return YamlError.OutOfMemory;
                        YamlSerializer.writeYamlNode(alloc, buf, entry.value, indent + 2)?;
                    } else {
                        buf.appendSlice(alloc, ": ") catch return YamlError.OutOfMemory;
                        YamlSerializer.writeYamlScalar(alloc, buf, entry.value)?;
                    }
                    idx += 1;
                }
            },
            else => {
                YamlSerializer.writeYamlScalar(alloc, buf, val)?;
            },
        }
    }

    fn writeYamlMappingInlineFirst(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), entries: []YamlEntry, indent: usize): ! {
        let idx: usize = 0;
        for (entry of entries) {
            if (idx > 0) {
                buf.append(alloc, '\n') catch return YamlError.OutOfMemory;
                YamlSerializer.writeIndent(alloc, buf, indent)?;
            }
            YamlSerializer.writeYamlKey(alloc, buf, entry.key)?;
            if ((entry.value.kind == .Mapping && entry.value.map_val.len > 0) ||
                (entry.value.kind == .Sequence && entry.value.seq_val.len > 0))
            {
                buf.appendSlice(alloc, ":\n") catch return YamlError.OutOfMemory;
                YamlSerializer.writeYamlNode(alloc, buf, entry.value, indent + 2)?;
            } else {
                buf.appendSlice(alloc, ": ") catch return YamlError.OutOfMemory;
                YamlSerializer.writeYamlScalar(alloc, buf, entry.value)?;
            }
            idx += 1;
        }
    }

    fn writeJsonNode(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), val: YamlValue): ! {
        switch (val.kind) {
            .Null => buf.appendSlice(alloc, "null") catch return YamlError.OutOfMemory,
            .Bool => {
                if (val.bool_val) {
                    buf.appendSlice(alloc, "true") catch return YamlError.OutOfMemory;
                } else {
                    buf.appendSlice(alloc, "false") catch return YamlError.OutOfMemory;
                }
            },
            .Int => {
                let num_buf: [32]u8 = undefined;
                const num_str = std.fmt.bufPrint(&num_buf, "{d}", .{val.int_val}) catch return YamlError.OutOfMemory;
                buf.appendSlice(alloc, num_str) catch return YamlError.OutOfMemory;
            },
            .Float => {
                if (std.math.isNan(val.float_val) || std.math.isInf(val.float_val)) {
                    buf.appendSlice(alloc, "null") catch return YamlError.OutOfMemory;
                } else {
                    let f_buf: [64]u8 = undefined;
                    const f_str = std.fmt.bufPrint(&f_buf, "{d}", .{val.float_val}) catch return YamlError.OutOfMemory;
                    buf.appendSlice(alloc, f_str) catch return YamlError.OutOfMemory;
                }
            },
            .String => YamlSerializer.writeQuotedString(alloc, buf, val.str_val)?,
            .Sequence => {
                buf.append(alloc, '[') catch return YamlError.OutOfMemory;
                let idx: usize = 0;
                for (item of val.seq_val) {
                    if (idx > 0) buf.append(alloc, ',') catch return YamlError.OutOfMemory;
                    YamlSerializer.writeJsonNode(alloc, buf, item)?;
                    idx += 1;
                }
                buf.append(alloc, ']') catch return YamlError.OutOfMemory;
            },
            .Mapping => {
                buf.append(alloc, '{') catch return YamlError.OutOfMemory;
                let idx: usize = 0;
                for (entry of val.map_val) {
                    if (idx > 0) buf.append(alloc, ',') catch return YamlError.OutOfMemory;
                    YamlSerializer.writeQuotedString(alloc, buf, entry.key)?;
                    buf.append(alloc, ':') catch return YamlError.OutOfMemory;
                    YamlSerializer.writeJsonNode(alloc, buf, entry.value)?;
                    idx += 1;
                }
                buf.append(alloc, '}') catch return YamlError.OutOfMemory;
            },
        }
    }
}
