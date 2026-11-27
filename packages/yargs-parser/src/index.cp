// @cap/yargs-parser — Zero-allocation CLI argument parser for CP
// Clean-room MIT implementation with parity to npm `yargs-parser`.
// Reuses `@cap/camelcase` and `@cap/decamelize` for automatic camelCase/kebab-case key lookup.

import { bytesEq, findByteInBytes } from "cap:bin";
import { writeStr, writeI64, parseI64 } from "cap:fmt";
import { toCamelCase } from "../../ident_case/src/index.cp";
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

fn ypIsNegativeNumber(s: string): bool {
    if (s.len < 2 || s[0] != 45 || s[1] < 48 || s[1] > 57) {
        return false;
    }
    let i: usize = 1;
    while (i < s.len) {
        if (s[i] < 48 || s[i] > 57) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn ypFormatInt(valIn: i64, out: []u8): i64 {
    let tmp: [32]u8 = undefined;
    const n = writeI64(valIn, tmp[0..], 0);
    if (out.len < n) {
        return -1;
    }
    return writeStr(tmp[0..n], out, 0) as i64;
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
    if (bytesEq(entry.key[0..entry.key_len], query)) {
        return true;
    }
    if (entry.camel_len > 0 && bytesEq(entry.camel_key[0..entry.camel_len], query)) {
        return true;
    }
    if (entry.kebab_len > 0 && bytesEq(entry.kebab_key[0..entry.kebab_len], query)) {
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
        if (bytesEq(s, "false") || bytesEq(s, "0") || s.len == 0) {
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
            return writeStr(e.str_val[0..e.str_len], out, 0) as i64;
        }
        if (e.kind == ARG_BOOL) {
            const lit = if (e.bool_val) "true" else "false";
            if (out.len < lit.len) {
                return -1;
            }
            return writeStr(lit, out, 0) as i64;
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
        return writeStr(item.data[0..item.len], out, 0);
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
        return writeStr(item.data[0..item.len], out, 0);
    }
}

fn ypPushPositional(args: *ParsedArgs, s: string) {
    if (args.pos_count >= YP_MAX_POSITIONALS) {
        return;
    }
    let item = ypEmptyItem();
    item.len = writeStr(s, item.data[0..YP_VAL_CAP], 0);
    args.positionals[args.pos_count] = item;
    args.pos_count = args.pos_count + 1;
}

fn ypPushRest(args: *ParsedArgs, s: string) {
    if (args.rest_count >= YP_MAX_REST) {
        return;
    }
    let item = ypEmptyItem();
    item.len = writeStr(s, item.data[0..YP_VAL_CAP], 0);
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
    entry.key_len = writeStr(rawKey, entry.key[0..YP_KEY_CAP], 0);
    entry.camel_len = toCamelCase(rawKey, entry.camel_key[0..YP_KEY_CAP]);
    entry.kebab_len = decamelize(rawKey, "-", entry.kebab_key[0..YP_KEY_CAP]);
    entry.kind = kind;
    entry.bool_val = boolVal;
    entry.int_val = intVal;
    if (kind == ARG_STRING) {
        entry.str_len = writeStr(strVal, entry.str_val[0..YP_VAL_CAP], 0);
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
    if (bytesEq(rawVal, "true")) {
        ypSetEntry(args, rawKey, ARG_BOOL, true, 1, "");
        return;
    }
    if (bytesEq(rawVal, "false")) {
        ypSetEntry(args, rawKey, ARG_BOOL, false, 0, "");
        return;
    }
    if (v := parseI64(rawVal)) {
        ypSetEntry(args, rawKey, ARG_INT, v != 0, v, "");
        return;
    }
    ypSetEntry(args, rawKey, ARG_STRING, true, 0, rawVal);
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
        if (bytesEq(tok, "--")) {
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
            const eqIdx = findByteInBytes(tok, 61, 2) ?? tok.len;
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
            if (i + 1 < argv.len && !ypIsFlagToken(argv[i + 1]) && !bytesEq(argv[i + 1], "--")) {
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
            const eqIdx = findByteInBytes(tok, 61, 1) ?? tok.len;
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
            if (i + 1 < argv.len && !ypIsFlagToken(argv[i + 1]) && !bytesEq(argv[i + 1], "--")) {
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
