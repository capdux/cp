// @cap/yargs-parser — Zero-allocation CLI argument parser for CP
// Clean-room MIT implementation with parity to npm `yargs-parser`.
// Reuses `@cap/camelcase` and `@cap/decamelize` for automatic camelCase/kebab-case key lookup.

import { toCamelCase } from "../../camelcase/src/index.cp";
import { decamelize } from "../../decamelize/src/core.cp";

pub const ARG_BOOL: u8 = 1;
pub const ARG_INT: u8 = 2;
pub const ARG_STRING: u8 = 3;

const YP_MAX_ENTRIES: usize = 32;
const YP_MAX_POSITIONALS: usize = 32;
const YP_MAX_REST: usize = 16;
const YP_KEY_CAP: usize = 64;
const YP_VAL_CAP: usize = 128;

/// Fixed-size stack string item used for positional and `--` rest arguments.
pub struct ParsedItem {
    pub data: [128]u8,
    pub len: usize,
}

/// Single parsed CLI option entry with original, camelCase, and kebab-case keys.
pub struct ParsedEntry {
    pub key: [64]u8,
    pub key_len: usize,
    pub camel_key: [64]u8,
    pub camel_len: usize,
    pub kebab_key: [64]u8,
    pub kebab_len: usize,
    pub kind: u8,
    pub bool_val: bool,
    pub int_val: i64,
    pub str_val: [128]u8,
    pub str_len: usize,
}

fn ypStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn ypCopyToBuf(dst: []u8, src: string): usize {
    let n = src.len;
    if (n > dst.len) {
        n = dst.len;
    }
    let i: usize = 0;
    while (i < n) {
        dst[i] = src[i];
        i = i + 1;
    }
    return n;
}

fn ypIsDigit(ch: u8): bool {
    return ch >= 48 && ch <= 57;
}

fn ypIsNegativeNumber(s: string): bool {
    if (s.len < 2 || s[0] != 45 || !ypIsDigit(s[1])) {
        return false;
    }
    let i: usize = 1;
    while (i < s.len) {
        if (!ypIsDigit(s[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

struct YpIntParse {
    ok: bool,
    val: i64,
}

fn ypParseInt(s: string): YpIntParse {
    if (s.len == 0) {
        return YpIntParse { ok: false, val: 0 };
    }
    let i: usize = 0;
    let neg = false;
    if (s[0] == 45) {
        neg = true;
        i = 1;
    } else if (s[0] == 43) {
        i = 1;
    }
    if (i >= s.len || (s.len - i) > 18) {
        return YpIntParse { ok: false, val: 0 };
    }
    let acc: i64 = 0;
    while (i < s.len) {
        if (!ypIsDigit(s[i])) {
            return YpIntParse { ok: false, val: 0 };
        }
        acc = (acc * 10) + ((s[i] - 48) as i64);
        i = i + 1;
    }
    if (neg) {
        return YpIntParse { ok: true, val: -acc };
    }
    return YpIntParse { ok: true, val: acc };
}

fn ypFormatInt(valIn: i64, out: []u8): i64 {
    let tmp: [32]u8 = undefined;
    let neg = false;
    let mag: u64 = 0;
    if (valIn < 0) {
        neg = true;
        mag = (-valIn) as u64;
    } else {
        mag = valIn as u64;
    }
    let n: usize = 0;
    if (mag == 0) {
        tmp[0] = 48;
        n = 1;
    } else {
        while (mag > 0 && n < 30) {
            tmp[n] = ((mag % 10) as u8) + 48;
            mag = mag / 10;
            n = n + 1;
        }
    }
    const total = if (neg) n + 1 else n;
    if (out.len < total) {
        return -1;
    }
    let pos: usize = 0;
    if (neg) {
        out[pos] = 45;
        pos = pos + 1;
    }
    let k: usize = n;
    while (k > 0) {
        k = k - 1;
        out[pos] = tmp[k];
        pos = pos + 1;
    }
    return pos as i64;
}

fn ypIsFlagToken(s: string): bool {
    if (s.len < 2 || s[0] != 45) {
        return false;
    }
    if (ypIsNegativeNumber(s)) {
        return false;
    }
    return true;
}

fn ypEmptyItem(): ParsedItem {
    let data: [128]u8 = undefined;
    data[0] = 0;
    return ParsedItem { data: data, len: 0 };
}

fn ypEmptyEntry(): ParsedEntry {
    let key: [64]u8 = undefined;
    let camel_key: [64]u8 = undefined;
    let kebab_key: [64]u8 = undefined;
    let str_val: [128]u8 = undefined;
    key[0] = 0;
    camel_key[0] = 0;
    kebab_key[0] = 0;
    str_val[0] = 0;
    return ParsedEntry {
        key: key,
        key_len: 0,
        camel_key: camel_key,
        camel_len: 0,
        kebab_key: kebab_key,
        kebab_len: 0,
        kind: ARG_BOOL,
        bool_val: false,
        int_val: 0,
        str_val: str_val,
        str_len: 0,
    };
}

fn ypEntryMatchesKey(entry: ParsedEntry, query: string): bool {
    if (ypStrEq(entry.key[0..entry.key_len], query)) {
        return true;
    }
    if (entry.camel_len > 0 && ypStrEq(entry.camel_key[0..entry.camel_len], query)) {
        return true;
    }
    if (entry.kebab_len > 0 && ypStrEq(entry.kebab_key[0..entry.kebab_len], query)) {
        return true;
    }
    return false;
}

/// Zero-allocation parsed CLI arguments container.
pub struct ParsedArgs {
    pub entries: [32]ParsedEntry,
    pub entry_count: usize,
    pub positionals: [32]ParsedItem,
    pub pos_count: usize,
    pub rest: [16]ParsedItem,
    pub rest_count: usize,

    /// Find option entry index matching `key` (original, camelCase, or kebab-case), or `-1`.
    pub fn findIndex(key: string): i64 {
        let i: usize = 0;
        while (i < _.entry_count) {
            if (ypEntryMatchesKey(_.entries[i], key)) {
                return i as i64;
            }
            i = i + 1;
        }
        return -1;
    }

    /// Return `true` if option `key` (raw, camelCase, or kebab-case) was provided.
    pub fn has(key: string): bool {
        return _.findIndex(key) >= 0;
    }

    /// Retrieve boolean option value, or `default_val` if absent.
    pub fn getBool(key: string, default_val: bool): bool {
        const idx = _.findIndex(key);
        if (idx < 0) {
            return default_val;
        }
        const e = _.entries[idx as usize];
        if (e.kind == ARG_BOOL) {
            return e.bool_val;
        }
        if (e.kind == ARG_INT) {
            return e.int_val != 0;
        }
        const s = e.str_val[0..e.str_len];
        if (ypStrEq(s, "false") || ypStrEq(s, "0") || s.len == 0) {
            return false;
        }
        return true;
    }

    /// Retrieve integer option value, or `default_val` if absent or non-numeric.
    pub fn getInt(key: string, default_val: i64): i64 {
        const idx = _.findIndex(key);
        if (idx < 0) {
            return default_val;
        }
        const e = _.entries[idx as usize];
        if (e.kind == ARG_INT) {
            return e.int_val;
        }
        if (e.kind == ARG_BOOL) {
            return if (e.bool_val) 1 else 0;
        }
        return default_val;
    }

    /// Retrieve string representation of option `key` into `out`.
    /// Returns bytes written on success, or `-1` if absent or `out` is too small.
    pub fn getString(key: string, out: []u8): i64 {
        const idx = _.findIndex(key);
        if (idx < 0) {
            return -1;
        }
        const e = _.entries[idx as usize];
        if (e.kind == ARG_STRING) {
            if (out.len < e.str_len) {
                return -1;
            }
            let i: usize = 0;
            while (i < e.str_len) {
                out[i] = e.str_val[i];
                i = i + 1;
            }
            return e.str_len as i64;
        }
        if (e.kind == ARG_BOOL) {
            const lit = if (e.bool_val) "true" else "false";
            if (out.len < lit.len) {
                return -1;
            }
            return ypCopyToBuf(out, lit) as i64;
        }
        return ypFormatInt(e.int_val, out);
    }

    /// Return the number of parsed option entries.
    pub fn entryCount(): usize {
        return _.entry_count;
    }

    /// Return the number of positional arguments.
    pub fn positionalCount(): usize {
        return _.pos_count;
    }

    /// Copy positional argument at `idx` into `out`. Returns bytes written (`0` if invalid).
    pub fn positionalAt(idx: usize, out: []u8): usize {
        if (idx >= _.pos_count) {
            return 0;
        }
        const item = _.positionals[idx];
        if (out.len < item.len) {
            return 0;
        }
        let i: usize = 0;
        while (i < item.len) {
            out[i] = item.data[i];
            i = i + 1;
        }
        return item.len;
    }

    /// Return the number of `--` passthrough arguments.
    pub fn restCount(): usize {
        return _.rest_count;
    }

    /// Copy `--` passthrough argument at `idx` into `out`. Returns bytes written (`0` if invalid).
    pub fn restAt(idx: usize, out: []u8): usize {
        if (idx >= _.rest_count) {
            return 0;
        }
        const item = _.rest[idx];
        if (out.len < item.len) {
            return 0;
        }
        let i: usize = 0;
        while (i < item.len) {
            out[i] = item.data[i];
            i = i + 1;
        }
        return item.len;
    }
}

fn ypPushPositional(args: *ParsedArgs, s: string) {
    if (args.pos_count >= YP_MAX_POSITIONALS) {
        return;
    }
    let item = ypEmptyItem();
    item.len = ypCopyToBuf(item.data[0..YP_VAL_CAP], s);
    args.positionals[args.pos_count] = item;
    args.pos_count = args.pos_count + 1;
}

fn ypPushRest(args: *ParsedArgs, s: string) {
    if (args.rest_count >= YP_MAX_REST) {
        return;
    }
    let item = ypEmptyItem();
    item.len = ypCopyToBuf(item.data[0..YP_VAL_CAP], s);
    args.rest[args.rest_count] = item;
    args.rest_count = args.rest_count + 1;
}

fn ypSetEntry(
    args: *ParsedArgs,
    rawKey: string,
    kind: u8,
    boolVal: bool,
    intVal: i64,
    strVal: string,
) {
    if (rawKey.len == 0) {
        return;
    }
    let entry = ypEmptyEntry();
    entry.key_len = ypCopyToBuf(entry.key[0..YP_KEY_CAP], rawKey);
    entry.camel_len = toCamelCase(rawKey, entry.camel_key[0..YP_KEY_CAP]);
    entry.kebab_len = decamelize(rawKey, "-", entry.kebab_key[0..YP_KEY_CAP]);
    entry.kind = kind;
    entry.bool_val = boolVal;
    entry.int_val = intVal;
    if (kind == ARG_STRING) {
        entry.str_len = ypCopyToBuf(entry.str_val[0..YP_VAL_CAP], strVal);
    }

    // Override existing entry if key (or camel/kebab alias) already present
    let i: usize = 0;
    while (i < args.entry_count) {
        if (ypEntryMatchesKey(args.entries[i], rawKey)
            || (entry.camel_len > 0 && ypEntryMatchesKey(args.entries[i], entry.camel_key[0..entry.camel_len]))) {
            args.entries[i] = entry;
            return;
        }
        i = i + 1;
    }

    if (args.entry_count < YP_MAX_ENTRIES) {
        args.entries[args.entry_count] = entry;
        args.entry_count = args.entry_count + 1;
    }
}

fn ypSetCoerced(args: *ParsedArgs, rawKey: string, rawVal: string) {
    if (ypStrEq(rawVal, "true")) {
        ypSetEntry(args, rawKey, ARG_BOOL, true, 1, "");
        return;
    }
    if (ypStrEq(rawVal, "false")) {
        ypSetEntry(args, rawKey, ARG_BOOL, false, 0, "");
        return;
    }
    const parsedInt = ypParseInt(rawVal);
    if (parsedInt.ok) {
        ypSetEntry(args, rawKey, ARG_INT, parsedInt.val != 0, parsedInt.val, "");
        return;
    }
    ypSetEntry(args, rawKey, ARG_STRING, true, 0, rawVal);
}

fn ypFindEq(s: string, start: usize): usize {
    let i = start;
    while (i < s.len) {
        if (s[i] == 61) {
            return i;
        }
        i = i + 1;
    }
    return s.len;
}

/// Parse a CLI argument slice `argv` into a zero-allocation `ParsedArgs` struct.
pub fn parseArgs(argv: []const string): ParsedArgs {
    let entries: [32]ParsedEntry = undefined;
    let positionals: [32]ParsedItem = undefined;
    let rest: [16]ParsedItem = undefined;
    let args = ParsedArgs {
        entries: entries,
        entry_count: 0,
        positionals: positionals,
        pos_count: 0,
        rest: rest,
        rest_count: 0,
    };

    let i: usize = 0;
    while (i < argv.len) {
        const tok = argv[i];

        // `--` end-of-options terminator
        if (ypStrEq(tok, "--")) {
            i = i + 1;
            while (i < argv.len) {
                ypPushRest(&args, argv[i]);
                ypPushPositional(&args, argv[i]);
                i = i + 1;
            }
            break;
        }

        // Long option `--...`
        if (tok.len > 2 && tok[0] == 45 && tok[1] == 45) {
            const eqIdx = ypFindEq(tok, 2);
            if (eqIdx < tok.len) {
                const key = tok[2..eqIdx];
                const val = tok[(eqIdx + 1)..tok.len];
                ypSetCoerced(&args, key, val);
                i = i + 1;
                continue;
            }

            // `--no-flag` boolean negation
            if (tok.len > 5 && tok[2] == 110 && tok[3] == 111 && tok[4] == 45) {
                const key = tok[5..tok.len];
                ypSetEntry(&args, key, ARG_BOOL, false, 0, "");
                i = i + 1;
                continue;
            }

            const key = tok[2..tok.len];
            if (i + 1 < argv.len && !ypIsFlagToken(argv[i + 1]) && !ypStrEq(argv[i + 1], "--")) {
                ypSetCoerced(&args, key, argv[i + 1]);
                i = i + 2;
            } else {
                ypSetEntry(&args, key, ARG_BOOL, true, 1, "");
                i = i + 1;
            }
            continue;
        }

        // Short option(s) `-f`, `-abc`, `-p 8080`, `-p=8080`
        if (ypIsFlagToken(tok)) {
            const eqIdx = ypFindEq(tok, 1);
            if (eqIdx < tok.len) {
                const key = tok[1..eqIdx];
                const val = tok[(eqIdx + 1)..tok.len];
                ypSetCoerced(&args, key, val);
                i = i + 1;
                continue;
            }

            // Combined short flags `-abc`: expand leading flags as true
            let k: usize = 1;
            while (k + 1 < tok.len) {
                ypSetEntry(&args, tok[k..(k + 1)], ARG_BOOL, true, 1, "");
                k = k + 1;
            }
            const lastKey = tok[k..(k + 1)];
            if (i + 1 < argv.len && !ypIsFlagToken(argv[i + 1]) && !ypStrEq(argv[i + 1], "--")) {
                ypSetCoerced(&args, lastKey, argv[i + 1]);
                i = i + 2;
            } else {
                ypSetEntry(&args, lastKey, ARG_BOOL, true, 1, "");
                i = i + 1;
            }
            continue;
        }

        // Positional argument (including `-` and negative numbers like `-42`)
        ypPushPositional(&args, tok);
        i = i + 1;
    }

    return args;
}

export {
    ARG_BOOL,
    ARG_INT,
    ARG_STRING,
    ParsedItem,
    ParsedEntry,
    ParsedArgs,
    parseArgs,
};
