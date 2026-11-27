// Source: cp/lib/ldap/ldap.cp (Pure CP RFC 4515 LDAP Search Filter Parser, Escaping, Evaluation & RFC 4514 DN)
// Zero external dependencies; pure CP & Zig std.

const std = @import("std");

type StrSlice = []const u8;

pub const DEFAULT_MAX_DEPTH: usize = 64;

pub err LdapError {
    InvalidFilter,
    UnbalancedParens,
    InvalidEscape,
    InvalidAttribute,
    MaxDepthExceeded,
    InvalidDn,
    OutOfMemory,
}

pub struct LdapAttr {
    name: []const u8,
    values: []const []const u8,
}

pub struct DnComponent {
    rdn_attr: []const u8,
    rdn_value: []const u8,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.rdn_attr.len > 0) allocator.free(_.rdn_attr);
        if (_.rdn_value.len > 0) allocator.free(_.rdn_value);
        _.rdn_attr = "";
        _.rdn_value = "";
    }
}

pub fn freeDn(allocator: std.mem.Allocator, components: []DnComponent) {
    for (c of components) {
        if (c.rdn_attr.len > 0) allocator.free(c.rdn_attr);
        if (c.rdn_value.len > 0) allocator.free(c.rdn_value);
    }
    allocator.free(components);
}

pub struct LdapAttrVal {
    attr: []const u8,
    val: []const u8,
}

pub struct LdapSubstring {
    attr: []const u8,
    initial: ([]const u8)?,
    any: [][]const u8,
    final: ([]const u8)?,
}

pub enum LdapFilter {
    and_list: []LdapFilter,
    or_list: []LdapFilter,
    not_filter: *LdapFilter,
    present: []const u8,
    eq: LdapAttrVal,
    approx: LdapAttrVal,
    ge: LdapAttrVal,
    le: LdapAttrVal,
    substring: LdapSubstring,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        switch (_.*) {
            .and_list list => {
                let i: usize = 0;
                while (i < list.len) : (i += 1) {
                    list[i].deinit(allocator);
                }
                allocator.free(list);
            },
            .or_list list => {
                let i: usize = 0;
                while (i < list.len) : (i += 1) {
                    list[i].deinit(allocator);
                }
                allocator.free(list);
            },
            .not_filter sub => {
                sub.deinit(allocator);
                allocator.destroy(sub);
            },
            .present attr => {
                allocator.free(attr);
            },
            .eq kv => {
                allocator.free(kv.attr);
                allocator.free(kv.val);
            },
            .approx kv => {
                allocator.free(kv.attr);
                allocator.free(kv.val);
            },
            .ge kv => {
                allocator.free(kv.attr);
                allocator.free(kv.val);
            },
            .le kv => {
                allocator.free(kv.attr);
                allocator.free(kv.val);
            },
            .substring sub => {
                allocator.free(sub.attr);
                if (init_val := sub.initial) {
                    allocator.free(init_val);
                }
                for (a of sub.any) {
                    allocator.free(a);
                }
                allocator.free(sub.any);
                if (fin_val := sub.final) {
                    allocator.free(fin_val);
                }
            },
        }
    }
}

type LdapVec16 = @Vector(16, u8);

fn makeLdapHexLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0xFF; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= '0' && i <= '9') {
            lut[i] = @as(u8, @intCast(i - '0'));
        } else if (i >= 'a' && i <= 'f') {
            lut[i] = @as(u8, @intCast(i - 'a' + 10));
        } else if (i >= 'A' && i <= 'F') {
            lut[i] = @as(u8, @intCast(i - 'A' + 10));
        }
    }
    return lut;
}

fn makeLdapCharLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        let f: u8 = 0;
        const is_alpha = (i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z');
        const is_digit = (i >= '0' && i <= '9');
        if (is_alpha) f |= 1;
        if (is_digit) f |= 2;
        if (is_alpha || is_digit || i == '-' || i == '.' || i == ';') f |= 4;
        if (i == ',' || i == '+' || i == '"' || i == '\\' || i == '<' || i == '>' || i == ';' || i == '#' || i == '=' || i == ' ') f |= 8;
        if (i == '"' || i == '<' || i == '>' || i == ';' || i == '+' || i == '\n' || i == '\r' || i == 0) f |= 16;
        if (i == '\\' || i == '*' || i == '(' || i == ')' || i == 0) f |= 32;
        lut[i] = f;
    }
    return lut;
}

const LDAP_HEX_LUT: [u8; 256] = makeLdapHexLut();
const LDAP_CHAR_LUT: [u8; 256] = makeLdapCharLut();

fn isHexDigit(c: u8): bool {
    return LDAP_HEX_LUT[usize(c)] != 0xFF;
}

fn hexVal(c: u8): u8 {
    return LDAP_HEX_LUT[usize(c)];
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    const lo_A: LdapVec16 = @splat(65 as u8);
    const hi_Z: LdapVec16 = @splat(90 as u8);
    const case_bit: LdapVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: LdapVec16 = a[i .. i + 16][0..16].*;
        const vb: LdapVec16 = b[i .. i + 16][0..16].*;
        const ma = (va >= lo_A) & (va <= hi_Z);
        const mb = (vb >= lo_A) & (vb <= hi_Z);
        const la = @select(u8, ma, va | case_bit, va);
        const lb = @select(u8, mb, vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) return false;
    }
    while (i < a.len) : (i += 1) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    }
    return true;
}

fn isAlpha(c: u8): bool {
    return (LDAP_CHAR_LUT[usize(c)] & 1) != 0;
}

fn isDigit(c: u8): bool {
    return (LDAP_CHAR_LUT[usize(c)] & 2) != 0;
}

fn isValidAttrName(attr: []const u8): bool {
    if (attr.len == 0) return false;
    const first = attr[0];
    if ((LDAP_CHAR_LUT[usize(first)] & 3) == 0) return false;
    let i: usize = 0;
    while (i < attr.len) : (i += 1) {
        if ((LDAP_CHAR_LUT[usize(attr[i])] & 4) == 0) {
            return false;
        }
    }
    return true;
}

fn decodeFilterSegment(allocator: std.mem.Allocator, raw: []const u8): ([]u8)! {
    let buf: std.ArrayList(u8) = .empty;
    errdefer buf.deinit(allocator);

    let i: usize = 0;
    const vlp: LdapVec16 = @splat(40 as u8);
    const vrp: LdapVec16 = @splat(41 as u8);
    const vbs: LdapVec16 = @splat(92 as u8);
    while (i < raw.len) {
        const run_start = i;
        while (i + 16 <= raw.len) : (i += 16) {
            const v: LdapVec16 = raw[i .. i + 16][0..16].*;
            if (@reduce(.Or, (v == vlp) | (v == vrp) | (v == vbs))) break;
        }
        while (i < raw.len && raw[i] != '(' && raw[i] != ')' && raw[i] != '\\') : (i += 1) {}
        if (i > run_start) {
            buf.appendSlice(allocator, raw[run_start..i]) catch return LdapError.OutOfMemory;
        }
        if (i >= raw.len) break;
        const c = raw[i];
        if (c == '(' || c == ')') {
            return LdapError.UnbalancedParens;
        }
        // c == '\\'
        if (i + 2 >= raw.len) {
            return LdapError.InvalidEscape;
        }
        const h1 = LDAP_HEX_LUT[usize(raw[i + 1])];
        const h2 = LDAP_HEX_LUT[usize(raw[i + 2])];
        if (h1 == 0xFF || h2 == 0xFF) {
            return LdapError.InvalidEscape;
        }
        const byte_val: u8 = (h1 << 4) | h2;
        buf.append(allocator, byte_val) catch return LdapError.OutOfMemory;
        i += 3;
    }

    return buf.toOwnedSlice(allocator) catch return LdapError.OutOfMemory;
}

struct FilterParser {
    src: []const u8,
    pos: usize = 0,
    max_depth: usize = DEFAULT_MAX_DEPTH,

    fn parseTop(*_, allocator: std.mem.Allocator): LdapFilter! {
        if (_.src.len == 0) return LdapError.InvalidFilter;
        const f = _.parseFilterNode(allocator, 0)?;
        if (_.pos != _.src.len) {
            let mut_f = f;
            mut_f.deinit(allocator);
            return LdapError.UnbalancedParens;
        }
        return f;
    }

    fn parseFilterNode(*_, allocator: std.mem.Allocator, depth: usize): LdapFilter! {
        if (_.pos >= _.src.len || _.src[_.pos] != '(') {
            return LdapError.UnbalancedParens;
        }
        const next_depth = depth + 1;
        if (next_depth > _.max_depth) {
            return LdapError.MaxDepthExceeded;
        }
        _.pos += 1; // consume '('

        if (_.pos >= _.src.len) {
            return LdapError.UnbalancedParens;
        }

        const ch = _.src[_.pos];
        if (ch == '&') {
            _.pos += 1;
            const list = _.parseFilterList(allocator, next_depth)?;
            if (list.len == 0) {
                allocator.free(list);
                return LdapError.InvalidFilter;
            }
            if (_.pos >= _.src.len || _.src[_.pos] != ')') {
                let i: usize = 0;
                while (i < list.len) : (i += 1) {
                    list[i].deinit(allocator);
                }
                allocator.free(list);
                return LdapError.UnbalancedParens;
            }
            _.pos += 1;
            return LdapFilter{ .and_list = list };
        } else if (ch == '|') {
            _.pos += 1;
            const list = _.parseFilterList(allocator, next_depth)?;
            if (list.len == 0) {
                allocator.free(list);
                return LdapError.InvalidFilter;
            }
            if (_.pos >= _.src.len || _.src[_.pos] != ')') {
                let i: usize = 0;
                while (i < list.len) : (i += 1) {
                    list[i].deinit(allocator);
                }
                allocator.free(list);
                return LdapError.UnbalancedParens;
            }
            _.pos += 1;
            return LdapFilter{ .or_list = list };
        } else if (ch == '!') {
            _.pos += 1;
            if (_.pos >= _.src.len || _.src[_.pos] != '(') {
                return LdapError.InvalidFilter;
            }
            let sub = _.parseFilterNode(allocator, next_depth)?;
            if (_.pos >= _.src.len || _.src[_.pos] != ')') {
                sub.deinit(allocator);
                return LdapError.UnbalancedParens;
            }
            _.pos += 1;
            let ptr = allocator.create(LdapFilter) catch {
                sub.deinit(allocator);
                return LdapError.OutOfMemory;
            };
            ptr.* = sub;
            return LdapFilter{ .not_filter = ptr };
        }

        return _.parseItem(allocator);
    }

    fn parseFilterList(*_, allocator: std.mem.Allocator, depth: usize): ([]LdapFilter)! {
        let items: std.ArrayList(LdapFilter) = .empty;
        errdefer {
            let i: usize = 0;
            while (i < items.items.len) : (i += 1) {
                items.items[i].deinit(allocator);
            }
            items.deinit(allocator);
        }

        while (_.pos < _.src.len && _.src[_.pos] == '(') {
            const child = _.parseFilterNode(allocator, depth)?;
            items.append(allocator, child) catch {
                let mut_child = child;
                mut_child.deinit(allocator);
                return LdapError.OutOfMemory;
            };
        }

        if (_.pos < _.src.len && _.src[_.pos] != ')') {
            return LdapError.InvalidFilter;
        }

        return items.toOwnedSlice(allocator) catch return LdapError.OutOfMemory;
    }

    fn parseItem(*_, allocator: std.mem.Allocator): LdapFilter! {
        const attr_start = _.pos;
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == '=' || c == '~' || c == '>' || c == '<' || c == ')' || c == '(') {
                break;
            }
            _.pos += 1;
        }

        const attr_raw = _.src[attr_start.._.pos];
        if (!isValidAttrName(attr_raw)) {
            return LdapError.InvalidAttribute;
        }

        if (_.pos >= _.src.len) {
            return LdapError.UnbalancedParens;
        }

        let op_kind: u8 = 0; // 0: '=', 1: '~=', 2: '>=', 3: '<='
        const op_c = _.src[_.pos];
        if (op_c == '=') {
            op_kind = 0;
            _.pos += 1;
        } else if (op_c == '~' && _.pos + 1 < _.src.len && _.src[_.pos + 1] == '=') {
            op_kind = 1;
            _.pos += 2;
        } else if (op_c == '>' && _.pos + 1 < _.src.len && _.src[_.pos + 1] == '=') {
            op_kind = 2;
            _.pos += 2;
        } else if (op_c == '<' && _.pos + 1 < _.src.len && _.src[_.pos + 1] == '=') {
            op_kind = 3;
            _.pos += 2;
        } else {
            return LdapError.InvalidFilter;
        }

        const val_start = _.pos;
        while (_.pos < _.src.len && _.src[_.pos] != ')') {
            if (_.src[_.pos] == '(') {
                return LdapError.UnbalancedParens;
            }
            _.pos += 1;
        }

        if (_.pos >= _.src.len || _.src[_.pos] != ')') {
            return LdapError.UnbalancedParens;
        }

        const val_raw = _.src[val_start.._.pos];
        _.pos += 1; // consume ')'

        if (op_kind == 0 && val_raw.len == 1 && val_raw[0] == '*') {
            const attr_dup = allocator.dupe(u8, attr_raw) catch return LdapError.OutOfMemory;
            return LdapFilter{ .present = attr_dup };
        }

        if (op_kind == 0) {
            // Check if there is any unescaped '*'
            let has_star = false;
            for (c of val_raw) {
                if (c == '*') {
                    has_star = true;
                    break;
                }
            }
            if (has_star) {
                return _.parseSubstring(allocator, attr_raw, val_raw);
            }
        }

        const decoded_val = decodeFilterSegment(allocator, val_raw)?;
        errdefer allocator.free(decoded_val);
        const attr_dup = allocator.dupe(u8, attr_raw) catch return LdapError.OutOfMemory;

        if (op_kind == 0) {
            return LdapFilter{ .eq = LdapAttrVal{ attr: attr_dup, val: decoded_val } };
        } else if (op_kind == 1) {
            return LdapFilter{ .approx = LdapAttrVal{ attr: attr_dup, val: decoded_val } };
        } else if (op_kind == 2) {
            return LdapFilter{ .ge = LdapAttrVal{ attr: attr_dup, val: decoded_val } };
        } else {
            return LdapFilter{ .le = LdapAttrVal{ attr: attr_dup, val: decoded_val } };
        }
    }

    fn parseSubstring(*_, allocator: std.mem.Allocator, attr_raw: []const u8, val_raw: []const u8): LdapFilter! {
        let parts: std.ArrayList(StrSlice) = .empty;
        errdefer {
            for (p of parts.items) {
                allocator.free(p);
            }
            parts.deinit(allocator);
        }

        let seg_start: usize = 0;
        let i: usize = 0;
        while (i <= val_raw.len) : (i += 1) {
            if (i == val_raw.len || val_raw[i] == '*') {
                const decoded = decodeFilterSegment(allocator, val_raw[seg_start..i])?;
                parts.append(allocator, decoded) catch {
                    allocator.free(decoded);
                    return LdapError.OutOfMemory;
                };
                seg_start = i + 1;
            }
        }

        const total = parts.items.len;
        // total >= 2 because at least one '*' was found
        let initial: ([]const u8)? = null;
        if (parts.items[0].len > 0) {
            initial = parts.items[0];
        } else {
            allocator.free(parts.items[0]);
        }
        errdefer { if (iv := initial) { allocator.free(iv); } }

        let final_seg: ([]const u8)? = null;
        if (parts.items[total - 1].len > 0) {
            final_seg = parts.items[total - 1];
        } else {
            allocator.free(parts.items[total - 1]);
        }
        errdefer { if (fv := final_seg) { allocator.free(fv); } }

        let any_list: std.ArrayList(StrSlice) = .empty;
        errdefer {
            for (a of any_list.items) {
                allocator.free(a);
            }
            any_list.deinit(allocator);
        }

        let idx: usize = 1;
        while (idx + 1 < total) : (idx += 1) {
            if (parts.items[idx].len > 0) {
                any_list.append(allocator, parts.items[idx]) catch {
                    return LdapError.OutOfMemory;
                };
            } else {
                allocator.free(parts.items[idx]);
            }
        }

        // Clear parts backing buffer without freeing moved strings
        parts.deinit(allocator);

        const any_slice = any_list.toOwnedSlice(allocator) catch return LdapError.OutOfMemory;
        errdefer {
            for (a of any_slice) {
                allocator.free(a);
            }
            allocator.free(any_slice);
        }

        const attr_dup = allocator.dupe(u8, attr_raw) catch return LdapError.OutOfMemory;
        return LdapFilter{
            .substring = LdapSubstring{
                attr: attr_dup,
                initial: initial,
                any: any_slice,
                final: final_seg,
            },
        };
    }
}

pub fn parseFilter(allocator: std.mem.Allocator, expr: []const u8): LdapFilter! {
    return parseFilterWithMaxDepth(allocator, expr, DEFAULT_MAX_DEPTH);
}

pub fn parseFilterWithMaxDepth(allocator: std.mem.Allocator, expr: []const u8, max_depth: usize): LdapFilter! {
    let parser = FilterParser{
        src: expr,
        pos: 0,
        max_depth: max_depth,
    };
    return parser.parseTop(allocator);
}

struct FilterFormatter {
    buf: std.ArrayList(u8) = .empty,

    fn deinit(*_, allocator: std.mem.Allocator) {
        _.buf.deinit(allocator);
    }

    fn appendEscaped(*_, allocator: std.mem.Allocator, raw: []const u8): ! {
        let i: usize = 0;
        const vbs: LdapVec16 = @splat(92 as u8);
        const vst: LdapVec16 = @splat(42 as u8);
        const vlp: LdapVec16 = @splat(40 as u8);
        const vrp: LdapVec16 = @splat(41 as u8);
        const vnul: LdapVec16 = @splat(0 as u8);
        while (i < raw.len) {
            const run_start = i;
            while (i + 16 <= raw.len) : (i += 16) {
                const v: LdapVec16 = raw[i .. i + 16][0..16].*;
                if (@reduce(.Or, (v == vbs) | (v == vst) | (v == vlp) | (v == vrp) | (v == vnul))) break;
            }
            while (i < raw.len && (LDAP_CHAR_LUT[usize(raw[i])] & 32) == 0) : (i += 1) {}
            if (i > run_start) {
                _.buf.appendSlice(allocator, raw[run_start..i]) catch return LdapError.OutOfMemory;
            }
            if (i >= raw.len) break;
            const c = raw[i];
            i += 1;
            if (c == '\\') {
                _.buf.appendSlice(allocator, "\\5c") catch return LdapError.OutOfMemory;
            } else if (c == '*') {
                _.buf.appendSlice(allocator, "\\2a") catch return LdapError.OutOfMemory;
            } else if (c == '(') {
                _.buf.appendSlice(allocator, "\\28") catch return LdapError.OutOfMemory;
            } else if (c == ')') {
                _.buf.appendSlice(allocator, "\\29") catch return LdapError.OutOfMemory;
            } else {
                _.buf.appendSlice(allocator, "\\00") catch return LdapError.OutOfMemory;
            }
        }
    }

    fn writeFilter(*_, allocator: std.mem.Allocator, filter: LdapFilter): ! {
        switch (filter) {
            .and_list list => {
                _.buf.appendSlice(allocator, "(&") catch return LdapError.OutOfMemory;
                for (item of list) {
                    _.writeFilter(allocator, item)?;
                }
                _.buf.append(allocator, ')') catch return LdapError.OutOfMemory;
            },
            .or_list list => {
                _.buf.appendSlice(allocator, "(|") catch return LdapError.OutOfMemory;
                for (item of list) {
                    _.writeFilter(allocator, item)?;
                }
                _.buf.append(allocator, ')') catch return LdapError.OutOfMemory;
            },
            .not_filter sub => {
                _.buf.appendSlice(allocator, "(!") catch return LdapError.OutOfMemory;
                _.writeFilter(allocator, sub.*)?;
                _.buf.append(allocator, ')') catch return LdapError.OutOfMemory;
            },
            .present attr => {
                _.buf.append(allocator, '(') catch return LdapError.OutOfMemory;
                _.buf.appendSlice(allocator, attr) catch return LdapError.OutOfMemory;
                _.buf.appendSlice(allocator, "=*)") catch return LdapError.OutOfMemory;
            },
            .eq kv => {
                _.buf.append(allocator, '(') catch return LdapError.OutOfMemory;
                _.buf.appendSlice(allocator, kv.attr) catch return LdapError.OutOfMemory;
                _.buf.append(allocator, '=') catch return LdapError.OutOfMemory;
                _.appendEscaped(allocator, kv.val)?;
                _.buf.append(allocator, ')') catch return LdapError.OutOfMemory;
            },
            .approx kv => {
                _.buf.append(allocator, '(') catch return LdapError.OutOfMemory;
                _.buf.appendSlice(allocator, kv.attr) catch return LdapError.OutOfMemory;
                _.buf.appendSlice(allocator, "~=") catch return LdapError.OutOfMemory;
                _.appendEscaped(allocator, kv.val)?;
                _.buf.append(allocator, ')') catch return LdapError.OutOfMemory;
            },
            .ge kv => {
                _.buf.append(allocator, '(') catch return LdapError.OutOfMemory;
                _.buf.appendSlice(allocator, kv.attr) catch return LdapError.OutOfMemory;
                _.buf.appendSlice(allocator, ">=") catch return LdapError.OutOfMemory;
                _.appendEscaped(allocator, kv.val)?;
                _.buf.append(allocator, ')') catch return LdapError.OutOfMemory;
            },
            .le kv => {
                _.buf.append(allocator, '(') catch return LdapError.OutOfMemory;
                _.buf.appendSlice(allocator, kv.attr) catch return LdapError.OutOfMemory;
                _.buf.appendSlice(allocator, "<=") catch return LdapError.OutOfMemory;
                _.appendEscaped(allocator, kv.val)?;
                _.buf.append(allocator, ')') catch return LdapError.OutOfMemory;
            },
            .substring sub => {
                _.buf.append(allocator, '(') catch return LdapError.OutOfMemory;
                _.buf.appendSlice(allocator, sub.attr) catch return LdapError.OutOfMemory;
                _.buf.append(allocator, '=') catch return LdapError.OutOfMemory;
                if (iv := sub.initial) {
                    _.appendEscaped(allocator, iv)?;
                }
                _.buf.append(allocator, '*') catch return LdapError.OutOfMemory;
                for (a of sub.any) {
                    _.appendEscaped(allocator, a)?;
                    _.buf.append(allocator, '*') catch return LdapError.OutOfMemory;
                }
                if (fv := sub.final) {
                    _.appendEscaped(allocator, fv)?;
                }
                _.buf.append(allocator, ')') catch return LdapError.OutOfMemory;
            },
        }
    }
}

pub fn escapeFilterValue(allocator: std.mem.Allocator, raw: []const u8): ([]u8)! {
    let fmt = FilterFormatter{};
    errdefer fmt.deinit(allocator);
    fmt.appendEscaped(allocator, raw)?;
    return fmt.buf.toOwnedSlice(allocator) catch return LdapError.OutOfMemory;
}

pub fn formatFilter(allocator: std.mem.Allocator, filter: LdapFilter): ([]u8)! {
    let fmt = FilterFormatter{};
    errdefer fmt.deinit(allocator);
    fmt.writeFilter(allocator, filter)?;
    return fmt.buf.toOwnedSlice(allocator) catch return LdapError.OutOfMemory;
}

fn findAttr(entry_attrs: []const LdapAttr, name: []const u8): LdapAttr? {
    for (a of entry_attrs) {
        if (asciiEqlIgnoreCase(a.name, name)) {
            return a;
        }
    }
    return null;
}

fn compareAsciiIgnoreCase(a: []const u8, b: []const u8): i8 {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) : (i += 1) {
        const ca = toLowerAscii(a[i]);
        const cb = toLowerAscii(b[i]);
        if (ca < cb) return -1;
        if (ca > cb) return 1;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

fn compareAttrValues(a: []const u8, b: []const u8): i8 {
    const num_a = std.fmt.parseInt(i64, a, 10) catch null;
    const num_b = std.fmt.parseInt(i64, b, 10) catch null;
    if (na := num_a) {
        if (nb := num_b) {
            if (na < nb) return -1;
            if (na > nb) return 1;
            return 0;
        }
    }
    return compareAsciiIgnoreCase(a, b);
}

fn startsWithIgnoreCase(haystack: []const u8, needle: []const u8): bool {
    if (haystack.len < needle.len) return false;
    return asciiEqlIgnoreCase(haystack[0..needle.len], needle);
}

fn endsWithIgnoreCase(haystack: []const u8, needle: []const u8): bool {
    if (haystack.len < needle.len) return false;
    return asciiEqlIgnoreCase(haystack[haystack.len - needle.len .. haystack.len], needle);
}

fn indexOfIgnoreCase(haystack: []const u8, needle: []const u8, start: usize): usize? {
    if (needle.len == 0) return start;
    if (start + needle.len > haystack.len) return null;
    let i = start;
    const limit = haystack.len - needle.len;
    while (i <= limit) : (i += 1) {
        if (asciiEqlIgnoreCase(haystack[i .. i + needle.len], needle)) {
            return i;
        }
    }
    return null;
}

fn matchesSubstringValue(
    val: []const u8,
    initial: ([]const u8)?,
    any_segs: [][]const u8,
    final_seg: ([]const u8)?,
): bool {
    let pos: usize = 0;
    if (iv := initial) {
        if (!startsWithIgnoreCase(val, iv)) return false;
        pos = iv.len;
    }

    let end_limit = val.len;
    if (fv := final_seg) {
        if (val.len < pos + fv.len) return false;
        if (!endsWithIgnoreCase(val, fv)) return false;
        end_limit = val.len - fv.len;
    }

    const middle = val[0..end_limit];
    for (seg of any_segs) {
        if (found := indexOfIgnoreCase(middle, seg, pos)) {
            pos = found + seg.len;
        } else {
            return false;
        }
    }
    return true;
}

pub fn matchesEntry(filter: LdapFilter, entry_attrs: []const LdapAttr): bool {
    switch (filter) {
        .and_list list => {
            for (sub of list) {
                if (!matchesEntry(sub, entry_attrs)) return false;
            }
            return true;
        },
        .or_list list => {
            for (sub of list) {
                if (matchesEntry(sub, entry_attrs)) return true;
            }
            return false;
        },
        .not_filter sub => {
            return !matchesEntry(sub.*, entry_attrs);
        },
        .present attr => {
            if (found := findAttr(entry_attrs, attr)) {
                return found.values.len > 0;
            }
            return false;
        },
        .eq kv => {
            if (found := findAttr(entry_attrs, kv.attr)) {
                for (v of found.values) {
                    if (asciiEqlIgnoreCase(v, kv.val)) return true;
                }
            }
            return false;
        },
        .approx kv => {
            if (found := findAttr(entry_attrs, kv.attr)) {
                for (v of found.values) {
                    if (asciiEqlIgnoreCase(v, kv.val)) return true;
                }
            }
            return false;
        },
        .ge kv => {
            if (found := findAttr(entry_attrs, kv.attr)) {
                for (v of found.values) {
                    if (compareAttrValues(v, kv.val) >= 0) return true;
                }
            }
            return false;
        },
        .le kv => {
            if (found := findAttr(entry_attrs, kv.attr)) {
                for (v of found.values) {
                    if (compareAttrValues(v, kv.val) <= 0) return true;
                }
            }
            return false;
        },
        .substring sub => {
            if (found := findAttr(entry_attrs, sub.attr)) {
                for (v of found.values) {
                    if (matchesSubstringValue(v, sub.initial, sub.any, sub.final)) return true;
                }
            }
            return false;
        },
    }
}

fn isDnSpecialEscapable(c: u8): bool {
    return (LDAP_CHAR_LUT[usize(c)] & 8) != 0;
}

fn isDnForbiddenUnescaped(c: u8): bool {
    return (LDAP_CHAR_LUT[usize(c)] & 16) != 0;
}

pub fn parseDn(allocator: std.mem.Allocator, dn: []const u8): ([]DnComponent)! {
    if (dn.len == 0) {
        return allocator.alloc(DnComponent, 0) catch return LdapError.OutOfMemory;
    }

    let comps: std.ArrayList(DnComponent) = .empty;
    errdefer {
        for (c of comps.items) {
            allocator.free(c.rdn_attr);
            allocator.free(c.rdn_value);
        }
        comps.deinit(allocator);
    }

    let pos: usize = 0;
    while (pos < dn.len) {
        // Skip leading spaces before attribute
        while (pos < dn.len && dn[pos] == ' ') {
            pos += 1;
        }
        const attr_start = pos;
        while (pos < dn.len && dn[pos] != '=' && dn[pos] != ',' && dn[pos] != ' ') {
            pos += 1;
        }
        const attr_raw = dn[attr_start..pos];
        if (!isValidAttrName(attr_raw)) {
            return LdapError.InvalidDn;
        }

        // Skip optional spaces before '='
        while (pos < dn.len && dn[pos] == ' ') {
            pos += 1;
        }
        if (pos >= dn.len || dn[pos] != '=') {
            return LdapError.InvalidDn;
        }
        pos += 1; // consume '='

        // Skip leading unescaped spaces in value
        while (pos < dn.len && dn[pos] == ' ') {
            pos += 1;
        }

        let val_buf: std.ArrayList(u8) = .empty;
        errdefer val_buf.deinit(allocator);

        let last_non_trailing_space_len: usize = 0;
        let hit_comma = false;
        while (pos < dn.len) {
            const c = dn[pos];
            if (c == ',') {
                hit_comma = true;
                pos += 1;
                break;
            }
            if (c == '\\') {
                if (pos + 1 >= dn.len) {
                    return LdapError.InvalidDn;
                }
                const next = dn[pos + 1];
                if (isDnSpecialEscapable(next)) {
                    val_buf.append(allocator, next) catch return LdapError.OutOfMemory;
                    pos += 2;
                    last_non_trailing_space_len = val_buf.items.len;
                } else if (pos + 2 < dn.len && isHexDigit(next) && isHexDigit(dn[pos + 2])) {
                    const b: u8 = (hexVal(next) << 4) | hexVal(dn[pos + 2]);
                    val_buf.append(allocator, b) catch return LdapError.OutOfMemory;
                    pos += 3;
                    last_non_trailing_space_len = val_buf.items.len;
                } else {
                    return LdapError.InvalidDn;
                }
            } else if (isDnForbiddenUnescaped(c)) {
                return LdapError.InvalidDn;
            } else {
                val_buf.append(allocator, c) catch return LdapError.OutOfMemory;
                pos += 1;
                if (c != ' ') {
                    last_non_trailing_space_len = val_buf.items.len;
                }
            }
        }

        // Trim unescaped trailing spaces
        val_buf.shrinkRetainingCapacity(last_non_trailing_space_len);
        if (val_buf.items.len == 0) {
            return LdapError.InvalidDn;
        }

        // Trailing comma with nothing after it is invalid
        if (hit_comma && pos >= dn.len) {
            return LdapError.InvalidDn;
        }

        const attr_dup = allocator.dupe(u8, attr_raw) catch return LdapError.OutOfMemory;
        errdefer allocator.free(attr_dup);
        const val_dup = val_buf.toOwnedSlice(allocator) catch return LdapError.OutOfMemory;

        comps.append(allocator, DnComponent{
            rdn_attr: attr_dup,
            rdn_value: val_dup,
        }) catch {
            allocator.free(attr_dup);
            allocator.free(val_dup);
            return LdapError.OutOfMemory;
        };
    }

    return comps.toOwnedSlice(allocator) catch return LdapError.OutOfMemory;
}
