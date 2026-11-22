// Source: Absorbed from /data/dev/bun/src/bundler/defines.cp | Synced: 2026-11-22T01:36:00Z
// Pure-CP zero-allocation compile-time `--define` & `--drop` constant replacement table
// and side-effect purity flags for `cap:bundler`.

import std from "std";

pub const MAX_DEFINE_RULES: usize = 64;

/// Controls when a bare call to a defined symbol can be unwrapped/dropped during DCE.
pub enum CallUnwrapKind: u8 {
    never = 0,
    if_unused = 1,
    always = 2,
}

/// Compile-time replacement value kind parsed from `--define key=value`.
pub enum DefineValueKind: u8 {
    undefined_val = 0,
    null_val = 1,
    boolean = 2,
    number = 3,
    string_lit = 4,
    identifier = 5,
    raw_expr = 6,
}

/// Purity and drop optimization flags (mirrors Bun `DefineData.Flags`).
pub struct DefineFlags {
    valueless: bool = false,
    can_be_removed_if_unused: bool = false,
    call_can_be_unwrapped_if_unused: CallUnwrapKind = .never,
    method_call_must_be_replaced_with_undefined: bool = false,
}

/// Typed compile-time replacement value.
pub struct DefineValue {
    kind: DefineValueKind = .undefined_val,
    bool_val: bool = false,
    num_val: f64 = 0.0,
    str_val: []const u8 = "",

    pub static fn makeUndefined(): DefineValue {
        return DefineValue{ kind: .undefined_val };
    }

    pub static fn makeNull(): DefineValue {
        return DefineValue{ kind: .null_val };
    }

    pub static fn makeBool(v: bool): DefineValue {
        return DefineValue{ kind: .boolean, bool_val: v };
    }

    pub static fn makeNumber(v: f64, raw: []const u8): DefineValue {
        return DefineValue{ kind: .number, num_val: v, str_val: raw };
    }

    pub static fn makeString(s: []const u8): DefineValue {
        return DefineValue{ kind: .string_lit, str_val: s };
    }

    pub static fn makeIdentifier(ident: []const u8): DefineValue {
        return DefineValue{ kind: .identifier, str_val: ident };
    }

    /// Parse a `--define` right-hand-side expression into a typed `DefineValue`.
    pub static fn parse(raw: []const u8): DefineValue? {
        const s = trimSpace(raw);
        if (s.len == 0) return null;

        if (strEq(s, "undefined")) return DefineValue.makeUndefined();
        if (strEq(s, "null")) return DefineValue.makeNull();
        if (strEq(s, "true")) return DefineValue.makeBool(true);
        if (strEq(s, "false")) return DefineValue.makeBool(false);
        if (strEq(s, "NaN")) return DefineValue.makeNumber(std.math.nan(f64), "NaN");
        if (strEq(s, "Infinity")) return DefineValue.makeNumber(std.math.inf(f64), "Infinity");
        if (strEq(s, "-Infinity")) {
            const neg_inf: f64 = 0.0 - std.math.inf(f64);
            return DefineValue.makeNumber(neg_inf, "-Infinity");
        }

        // Quoted string literal (`"..."` or `'...'`)
        if (s.len >= 2 && (s[0] == '"' || s[0] == '\'')) {
            const q = s[0];
            if (s[s.len - 1] != q) return null;
            // Verify no unescaped matching quote inside
            let i: usize = 1;
            while (i + 1 < s.len) : (i += 1) {
                if (s[i] == '\\' && i + 2 < s.len) {
                    i += 1;
                    continue;
                }
                if (s[i] == q) return null;
            }
            return DefineValue.makeString(s[1 .. s.len - 1]);
        }

        // Numeric literal
        if (isNumericStart(s)) {
            const parsed_num = std.fmt.parseFloat(f64, s) catch return null;
            return DefineValue.makeNumber(parsed_num, s);
        }

        // Dotted identifier chain (e.g. `globalThis.fetch` or `myHelper`)
        if (isValidDefineKey(s)) {
            return DefineValue.makeIdentifier(s);
        }

        return null;
    }

    /// Format the value as canonical JS output into `out` (returns `null` if buffer too small).
    pub fn formatJs(out: []u8): ([]const u8)? {
        return switch (_.kind) {
            .undefined_val => writeLit(out, "undefined"),
            .null_val => writeLit(out, "null"),
            .boolean => writeLit(out, if (_.bool_val) "true" else "false"),
            .number => writeLit(out, if (_.str_val.len > 0) _.str_val else "0"),
            .identifier, .raw_expr => writeLit(out, _.str_val),
            .string_lit => formatQuotedJsString(_.str_val, out),
        };
    }
}

/// Single rule stored in `DefineTable`.
pub struct DefineEntry {
    key: []const u8 = "",
    value: DefineValue = DefineValue{},
    flags: DefineFlags = DefineFlags{},
    is_dot_chain: bool = false,
    active: bool = false,

    pub fn canBeRemovedIfUnused(): bool {
        return _.flags.can_be_removed_if_unused;
    }

    pub fn isValueless(): bool {
        return _.flags.valueless;
    }

    pub fn dropsMethodCall(): bool {
        return _.flags.method_call_must_be_replaced_with_undefined;
    }
}

fn trimSpace(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) : (start += 1) {}
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) : (end -= 1) {}
    return s[start..end];
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$';
}

fn isIdentContinue(c: u8): bool {
    return isIdentStart(c) || (c >= '0' && c <= '9');
}

fn isNumericStart(s: []const u8): bool {
    if (s.len == 0) return false;
    const c0 = s[0];
    if (c0 >= '0' && c0 <= '9') return true;
    if ((c0 == '-' || c0 == '+') && s.len >= 2 && s[1] >= '0' && s[1] <= '9') return true;
    return false;
}

/// Validates a JS identifier or dot-separated member chain (e.g. `process.env.NODE_ENV`).
pub fn isValidDefineKey(key: []const u8): bool {
    if (key.len == 0) return false;
    let pos: usize = 0;
    while (pos < key.len) {
        if (!isIdentStart(key[pos])) return false;
        pos += 1;
        while (pos < key.len && isIdentContinue(key[pos])) : (pos += 1) {}
        if (pos < key.len) {
            if (key[pos] != '.') return false;
            pos += 1;
            if (pos >= key.len) return false; // trailing dot rejected
        }
    }
    return true;
}

fn hasDot(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '.') return true;
    }
    return false;
}

fn writeLit(out: []u8, lit: []const u8): ([]const u8)? {
    if (out.len < lit.len) return null;
    let i: usize = 0;
    while (i < lit.len) : (i += 1) {
        out[i] = lit[i];
    }
    return out[0..lit.len];
}

fn formatQuotedJsString(s: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    if (pos >= out.len) return null;
    out[pos] = '"';
    pos += 1;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '"' || c == '\\') {
            if (pos + 2 > out.len) return null;
            out[pos] = '\\';
            out[pos + 1] = c;
            pos += 2;
        } else if (c == '\n') {
            if (pos + 2 > out.len) return null;
            out[pos] = '\\';
            out[pos + 1] = 'n';
            pos += 2;
        } else if (c == '\r') {
            if (pos + 2 > out.len) return null;
            out[pos] = '\\';
            out[pos + 1] = 'r';
            pos += 2;
        } else {
            if (pos + 1 > out.len) return null;
            out[pos] = c;
            pos += 1;
        }
    }
    if (pos >= out.len) return null;
    out[pos] = '"';
    pos += 1;
    return out[0..pos];
}

fn keyMatchesParts(key: []const u8, parts: []const []const u8): bool {
    if (parts.len == 0) return false;
    let pos: usize = 0;
    let p: usize = 0;
    while (p < parts.len) : (p += 1) {
        const seg = parts[p];
        if (seg.len == 0) return false;
        if (pos + seg.len > key.len) return false;
        if (!strEq(key[pos .. pos + seg.len], seg)) return false;
        pos += seg.len;
        if (p + 1 < parts.len) {
            if (pos >= key.len || key[pos] != '.') return false;
            pos += 1;
        }
    }
    return pos == key.len;
}

/// Zero-allocation compile-time `--define` & `--drop` table (up to 64 rules).
pub struct DefineTable {
    entries: [MAX_DEFINE_RULES]DefineEntry = [DefineEntry{}; MAX_DEFINE_RULES],
    len: usize = 0,

    pub static fn init(): DefineTable {
        return DefineTable{};
    }

    pub fn count(): usize {
        return _.len;
    }

    /// Insert or update a typed define entry with custom purity/drop flags.
    pub fn defineEntry(*_, key: []const u8, value: DefineValue, flags: DefineFlags): bool {
        const trimmed = trimSpace(key);
        if (!isValidDefineKey(trimmed)) return false;

        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (_.entries[i].active && strEq(_.entries[i].key, trimmed)) {
                _.entries[i].value = value;
                _.entries[i].flags = flags;
                return true;
            }
        }
        if (_.len >= MAX_DEFINE_RULES) return false;
        _.entries[_.len] = DefineEntry{
            key: trimmed,
            value: value,
            flags: flags,
            is_dot_chain: hasDot(trimmed),
            active: true,
        };
        _.len += 1;
        return true;
    }

    /// Parse `raw_value` and register `--define key=raw_value` with side-effect-free flags for literals.
    pub fn defineRaw(*_, key: []const u8, raw_value: []const u8): bool {
        const val = DefineValue.parse(raw_value) ?? return false;
        const is_pure_lit = val.kind != DefineValueKind.identifier && val.kind != DefineValueKind.raw_expr;
        return _.defineEntry(key, val, DefineFlags{
            can_be_removed_if_unused: is_pure_lit,
        });
    }

    /// Register a `--drop` rule (e.g. `console.log`), replacing method calls with `undefined`.
    pub fn addDrop(*_, key: []const u8): bool {
        return _.defineEntry(key, DefineValue.makeUndefined(), DefineFlags{
            can_be_removed_if_unused: true,
            call_can_be_unwrapped_if_unused: CallUnwrapKind.always,
            method_call_must_be_replaced_with_undefined: true,
        });
    }

    /// Exact lookup by identifier or dotted property key (e.g. `"process.env.NODE_ENV"`).
    pub fn lookup(key: []const u8): DefineEntry? {
        if (key.len == 0) return null;
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (_.entries[i].active && strEq(_.entries[i].key, key)) {
                return _.entries[i];
            }
        }
        return null;
    }

    /// Zero-allocation lookup by AST member chain segments (e.g. `["process", "env", "NODE_ENV"]`).
    pub fn lookupParts(parts: []const []const u8): DefineEntry? {
        if (parts.len == 0) return null;
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (_.entries[i].active && keyMatchesParts(_.entries[i].key, parts)) {
                return _.entries[i];
            }
        }
        return null;
    }

    /// Fast prefix check: returns true if any dot-chain rule starts with `prefix + "."`.
    pub fn hasDotPrefix(prefix: []const u8): bool {
        if (prefix.len == 0) return false;
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (!_.entries[i].active || !_.entries[i].is_dot_chain) continue;
            const k = _.entries[i].key;
            if (k.len > prefix.len && k[prefix.len] == '.' && strEq(k[0..prefix.len], prefix)) {
                return true;
            }
        }
        return false;
    }
}
