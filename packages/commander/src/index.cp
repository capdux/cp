// @cap/commander — Zero-allocation CLI command framework for CP
// Clean-room MIT implementation with parity to npm `commander` v12.
// Composes `@cap/yargs-parser` (argument parsing) and `@cap/cliui` (multi-column help layout)
// with zero duplication.

import {
    ARG_BOOL,
    ARG_INT,
    ARG_STRING,
    ParsedItem,
    ParsedEntry,
    ParsedArgs,
    parseArgs,
} from "../../yargs-parser/src/index.cp";
import {
    Column,
    CliUi,
    formatTwoCol,
} from "../../cliui/src/index.cp";

const CMD_MAX_OPTIONS: usize = 16;
const CMD_MAX_SUBCOMMANDS: usize = 16;
const CMD_MAX_ARGUMENTS: usize = 8;
const CMD_MAX_POSITIONALS: usize = 16;

fn cmdStrEq(a: string, b: string): bool {
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

fn cmdCopyBuf(dst: []u8, src: string): usize {
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

fn cmdAppendBytes(dst: []u8, pos: usize, s: string): usize {
    let w = pos;
    let i: usize = 0;
    while (i < s.len && w < dst.len) {
        dst[w] = s[i];
        w = w + 1;
        i = i + 1;
    }
    return w;
}

fn cmdAppendByte(dst: []u8, pos: usize, b: u8): usize {
    if (pos < dst.len) {
        dst[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn cmdIsSpace(ch: u8): bool {
    return ch == 32 || ch == 9 || ch == 10 || ch == 13;
}

fn cmdIsFlagSep(ch: u8): bool {
    return ch == 32 || ch == 9 || ch == 44 || ch == 124; // ' ', '\t', ',', '|'
}

fn cmdIsDigit(ch: u8): bool {
    return ch >= 48 && ch <= 57;
}

fn cmdTrim(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && cmdIsSpace(s[start])) {
        start = start + 1;
    }
    while (end > start && cmdIsSpace(s[end - 1])) {
        end = end - 1;
    }
    return s[start..end];
}

fn cmdHasChar(s: string, target: u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == target) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

struct CmdIntParse {
    ok: bool,
    val: i64,
}

fn cmdParseInt(s: string): CmdIntParse {
    if (s.len == 0) {
        return CmdIntParse { ok: false, val: 0 };
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
        return CmdIntParse { ok: false, val: 0 };
    }
    let acc: i64 = 0;
    while (i < s.len) {
        if (!cmdIsDigit(s[i])) {
            return CmdIntParse { ok: false, val: 0 };
        }
        acc = (acc * 10) + ((s[i] - 48) as i64);
        i = i + 1;
    }
    if (neg) {
        return CmdIntParse { ok: true, val: -acc };
    }
    return CmdIntParse { ok: true, val: acc };
}

fn cmdFormatInt(val_in: i64, out: []u8): usize {
    let tmp: [32]u8 = undefined;
    let neg = false;
    let mag: u64 = 0;
    if (val_in < 0) {
        neg = true;
        mag = (-val_in) as u64;
    } else {
        mag = val_in as u64;
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
    let pos: usize = 0;
    if (neg && pos < out.len) {
        out[pos] = 45;
        pos = pos + 1;
    }
    let k = n;
    while (k > 0 && pos < out.len) {
        k = k - 1;
        out[pos] = tmp[k];
        pos = pos + 1;
    }
    return pos;
}

fn cmdEmptyItem(): ParsedItem {
    let data: [128]u8 = undefined;
    data[0] = 0;
    return ParsedItem { data: data, len: 0 };
}

fn cmdMakeItem(s: string): ParsedItem {
    let item = cmdEmptyItem();
    item.len = cmdCopyBuf(item.data[0..128], s);
    return item;
}

fn cmdInsertEntry(
    parsed: *ParsedArgs,
    name: string,
    kind: u8,
    bool_val: bool,
    int_val: i64,
    str_val: string,
) {
    if (name.len == 0) {
        return;
    }
    const existing = parsed.findIndex(name);
    let key: [64]u8 = undefined;
    let camel_key: [64]u8 = undefined;
    let kebab_key: [64]u8 = undefined;
    let sbuf: [128]u8 = undefined;
    key[0] = 0;
    camel_key[0] = 0;
    kebab_key[0] = 0;
    sbuf[0] = 0;

    const klen = cmdCopyBuf(key[0..64], name);
    let slen: usize = 0;
    if (kind == ARG_STRING) {
        slen = cmdCopyBuf(sbuf[0..128], str_val);
    }

    const entry = ParsedEntry {
        key: key,
        key_len: klen,
        camel_key: camel_key,
        camel_len: 0,
        kebab_key: kebab_key,
        kebab_len: 0,
        kind: kind,
        bool_val: bool_val,
        int_val: int_val,
        str_val: sbuf,
        str_len: slen,
    };

    if (existing >= 0) {
        parsed.entries[existing as usize] = entry;
        return;
    }
    if (parsed.entry_count < 32) {
        parsed.entries[parsed.entry_count] = entry;
        parsed.entry_count = parsed.entry_count + 1;
    }
}

fn cmdSetDefaultEntry(parsed: *ParsedArgs, name: string, default_str: string) {
    if (cmdStrEq(default_str, "true")) {
        cmdInsertEntry(parsed, name, ARG_BOOL, true, 1, "");
        return;
    }
    if (cmdStrEq(default_str, "false")) {
        cmdInsertEntry(parsed, name, ARG_BOOL, false, 0, "");
        return;
    }
    const iv = cmdParseInt(default_str);
    if (iv.ok) {
        cmdInsertEntry(parsed, name, ARG_INT, iv.val != 0, iv.val, "");
        return;
    }
    cmdInsertEntry(parsed, name, ARG_STRING, true, 0, default_str);
}

/// Parsed representation of a Commander option flags string,
/// e.g. `"-p, --port <number>"`, `"-d, --debug"`, `"--no-color"`, `"[-f, --file [path]]"`.
pub struct FlagSpec {
    pub short_flag: u8,
    pub long_name: string,
    pub arg_name: string,
    pub takes_arg: bool,
    pub arg_required: bool,
    pub arg_optional: bool,
    pub negate: bool,
    pub variadic: bool,
}

/// Parse Commander option declaration syntax into a zero-allocation `FlagSpec`.
pub fn parseOptionFlags(flags: string): FlagSpec {
    let short_flag: u8 = 0;
    let long_name: string = "";
    let arg_name: string = "";
    let takes_arg = false;
    let arg_required = false;
    let arg_optional = false;
    let negate = false;
    let variadic = false;

    let s = cmdTrim(flags);
    // Unwrap outer optional brackets such as "[-f, --file [path]]"
    if (s.len >= 3 && s[0] == 91 && s[s.len - 1] == 93 && cmdHasChar(s, 45)) {
        s = cmdTrim(s[1..(s.len - 1)]);
    }

    let i: usize = 0;
    while (i < s.len) {
        while (i < s.len && (cmdIsFlagSep(s[i]) || s[i] == 61)) {
            i = i + 1;
        }
        if (i >= s.len) {
            break;
        }

        // Required option argument `<arg>` or `<arg...>`
        if (s[i] == 60) {
            let j = i + 1;
            while (j < s.len && s[j] != 62) {
                j = j + 1;
            }
            let inner = cmdTrim(s[(i + 1)..j]);
            if (inner.len >= 3
                && inner[inner.len - 1] == 46
                && inner[inner.len - 2] == 46
                && inner[inner.len - 3] == 46) {
                variadic = true;
                inner = inner[0..(inner.len - 3)];
            }
            arg_name = inner;
            takes_arg = true;
            arg_required = true;
            arg_optional = false;
            i = if (j < s.len) j + 1 else j;
            continue;
        }

        // Optional option argument `[arg]` or `[arg...]`
        if (s[i] == 91) {
            let j = i + 1;
            while (j < s.len && s[j] != 93) {
                j = j + 1;
            }
            let inner = cmdTrim(s[(i + 1)..j]);
            if (inner.len >= 3
                && inner[inner.len - 1] == 46
                && inner[inner.len - 2] == 46
                && inner[inner.len - 3] == 46) {
                variadic = true;
                inner = inner[0..(inner.len - 3)];
            }
            arg_name = inner;
            takes_arg = true;
            arg_required = false;
            arg_optional = true;
            i = if (j < s.len) j + 1 else j;
            continue;
        }

        // Long flag `--name` or `--no-name`
        if (i + 2 < s.len && s[i] == 45 && s[i + 1] == 45) {
            let j = i + 2;
            while (j < s.len && !cmdIsFlagSep(s[j]) && s[j] != 60 && s[j] != 91 && s[j] != 61) {
                j = j + 1;
            }
            const raw_long = s[(i + 2)..j];
            if (raw_long.len > 3 && raw_long[0] == 110 && raw_long[1] == 111 && raw_long[2] == 45) {
                negate = true;
                long_name = raw_long[3..raw_long.len];
            } else {
                long_name = raw_long;
            }
            i = j;
            continue;
        }

        // Short flag `-p`
        if (i + 1 < s.len && s[i] == 45 && s[i + 1] != 45) {
            short_flag = s[i + 1];
            let j = i + 2;
            while (j < s.len && !cmdIsFlagSep(s[j]) && s[j] != 60 && s[j] != 91 && s[j] != 61) {
                j = j + 1;
            }
            i = j;
            continue;
        }

        // Unrecognized token: advance to next separator
        while (i < s.len && !cmdIsFlagSep(s[i])) {
            i = i + 1;
        }
    }

    return FlagSpec {
        short_flag: short_flag,
        long_name: long_name,
        arg_name: arg_name,
        takes_arg: takes_arg,
        arg_required: arg_required,
        arg_optional: arg_optional,
        negate: negate,
        variadic: variadic,
    };
}

/// Registered option definition inside `Command`.
pub struct OptionDef {
    pub flags: string,
    pub desc: string,
    pub default_str: string,
    pub has_default: bool,
    pub required: bool,
    pub spec: FlagSpec,
}

/// Registered positional argument definition (`<required>` or `[optional]`).
pub struct ArgDef {
    pub spec: string,
    pub name: string,
    pub desc: string,
    pub required: bool,
    pub variadic: bool,
}

/// Parse a positional argument spec such as `"<source>"`, `"[dest]"`, `"<files...>"`.
pub fn parseArgSpec(spec: string, desc: string): ArgDef {
    let s = cmdTrim(spec);
    let required = true;
    let variadic = false;
    let inner = s;

    if (s.len >= 2 && s[0] == 60 && s[s.len - 1] == 62) {
        required = true;
        inner = cmdTrim(s[1..(s.len - 1)]);
    } else if (s.len >= 2 && s[0] == 91 && s[s.len - 1] == 93) {
        required = false;
        inner = cmdTrim(s[1..(s.len - 1)]);
    }

    if (inner.len >= 3
        && inner[inner.len - 1] == 46
        && inner[inner.len - 2] == 46
        && inner[inner.len - 3] == 46) {
        variadic = true;
        inner = inner[0..(inner.len - 3)];
    }

    return ArgDef {
        spec: spec,
        name: inner,
        desc: desc,
        required: required,
        variadic: variadic,
    };
}

/// Registered subcommand definition inside `Command`.
pub struct SubcommandDef {
    pub name: string,
    pub desc: string,
}

/// Zero-allocation result of `Command.parse(argv)`.
pub struct CommandParseResult {
    pub ok: bool,
    pub show_help: bool,
    pub show_version: bool,
    pub args: ParsedArgs,
    pub subcommand: [64]u8,
    pub subcommand_len: usize,
    pub positionals: [16]ParsedItem,
    pub pos_count: usize,
    pub error_msg: [192]u8,
    pub error_len: usize,

    /// Return `true` if a registered subcommand was matched.
    pub fn hasSubcommand(): bool {
        return _.subcommand_len > 0;
    }

    /// Copy matched subcommand name into `out`. Returns bytes written.
    pub fn subcommandName(out: []u8): usize {
        if (_.subcommand_len == 0) {
            return 0;
        }
        return cmdCopyBuf(out, _.subcommand[0.._.subcommand_len]);
    }

    /// Copy validation error message into `out`. Returns bytes written.
    pub fn errorMessage(out: []u8): usize {
        if (_.error_len == 0) {
            return 0;
        }
        return cmdCopyBuf(out, _.error_msg[0.._.error_len]);
    }

    /// Return `true` if option `name` is present (including defaults).
    pub fn hasOption(name: string): bool {
        return _.args.has(name);
    }

    /// Retrieve boolean option value, or `default_val` if absent.
    pub fn getBool(name: string, default_val: bool): bool {
        return _.args.getBool(name, default_val);
    }

    /// Retrieve integer option value, or `default_val` if absent.
    pub fn getInt(name: string, default_val: i64): i64 {
        return _.args.getInt(name, default_val);
    }

    /// Retrieve string option value into `out`. Returns bytes written or `-1`.
    pub fn getString(name: string, out: []u8): i64 {
        return _.args.getString(name, out);
    }

    /// Return number of positional arguments (excluding matched subcommand).
    pub fn argCount(): usize {
        return _.pos_count;
    }

    /// Copy positional argument at `idx` into `out`. Returns bytes written.
    pub fn argAt(idx: usize, out: []u8): usize {
        if (idx >= _.pos_count) {
            return 0;
        }
        const item = _.positionals[idx];
        return cmdCopyBuf(out, item.data[0..item.len]);
    }
}

fn cmdPushPos(pos_arr: []ParsedItem, count: usize, s: string): usize {
    if (count >= CMD_MAX_POSITIONALS) {
        return count;
    }
    pos_arr[count] = cmdMakeItem(s);
    return count + 1;
}

/// Zero-allocation Commander v12-style CLI command builder, parser, validator & help renderer.
pub struct Command {
    pub name: string,
    pub version_str: string,
    pub desc: string,
    options: [16]OptionDef,
    pub option_count: usize,
    subcommands: [16]SubcommandDef,
    pub subcommand_count: usize,
    arguments: [8]ArgDef,
    pub argument_count: usize,

    /// Create a new `Command` with the given program name.
    pub static fn new(name: string): Command {
        let options: [16]OptionDef = undefined;
        let subcommands: [16]SubcommandDef = undefined;
        let arguments: [8]ArgDef = undefined;
        return Command {
            name: name,
            version_str: "",
            desc: "",
            options: options,
            option_count: 0,
            subcommands: subcommands,
            subcommand_count: 0,
            arguments: arguments,
            argument_count: 0,
        };
    }

    /// Set the program version string (enables `-V, --version`).
    pub fn version(*_, v: string) {
        _.version_str = v;
    }

    /// Set the program description shown in `--help`.
    pub fn description(*_, d: string) {
        _.desc = d;
    }

    /// Register an option using Commander flags syntax (e.g. `"-p, --port <number>"`).
    /// Pass `""` for `default_str` if there is no default value.
    pub fn option(*_, flags: string, desc: string, default_str: string): bool {
        if (_.option_count >= CMD_MAX_OPTIONS) {
            return false;
        }
        const spec = parseOptionFlags(flags);
        _.options[_.option_count] = OptionDef {
            flags: flags,
            desc: desc,
            default_str: default_str,
            has_default: default_str.len > 0,
            required: false,
            spec: spec,
        };
        _.option_count = _.option_count + 1;
        return true;
    }

    /// Register a required option (triggers parse error when omitted).
    pub fn requiredOption(*_, flags: string, desc: string): bool {
        if (_.option_count >= CMD_MAX_OPTIONS) {
            return false;
        }
        const spec = parseOptionFlags(flags);
        _.options[_.option_count] = OptionDef {
            flags: flags,
            desc: desc,
            default_str: "",
            has_default: false,
            required: true,
            spec: spec,
        };
        _.option_count = _.option_count + 1;
        return true;
    }

    /// Register a positional argument (`"<required>"` or `"[optional]"`).
    pub fn argument(*_, spec: string, desc: string): bool {
        if (_.argument_count >= CMD_MAX_ARGUMENTS) {
            return false;
        }
        _.arguments[_.argument_count] = parseArgSpec(spec, desc);
        _.argument_count = _.argument_count + 1;
        return true;
    }

    /// Register a subcommand.
    pub fn subcommand(*_, name: string, desc: string): bool {
        if (_.subcommand_count >= CMD_MAX_SUBCOMMANDS) {
            return false;
        }
        _.subcommands[_.subcommand_count] = SubcommandDef {
            name: name,
            desc: desc,
        };
        _.subcommand_count = _.subcommand_count + 1;
        return true;
    }

    /// Parse `argv` via `@cap/yargs-parser`, apply Commander defaults/negations/aliases,
    /// route subcommands, and validate required options and `<required>` positional arguments.
    pub fn parse(argv: []const string): CommandParseResult {
        let parsed = parseArgs(argv);

        let sub_buf: [64]u8 = undefined;
        let err_buf: [192]u8 = undefined;
        let pos_items: [16]ParsedItem = undefined;
        sub_buf[0] = 0;
        err_buf[0] = 0;

        let res = CommandParseResult {
            ok: true,
            show_help: false,
            show_version: false,
            args: parsed,
            subcommand: sub_buf,
            subcommand_len: 0,
            positionals: pos_items,
            pos_count: 0,
            error_msg: err_buf,
            error_len: 0,
        };

        // Detect `-h` / `--help` and `-V` / `--version`
        if (res.args.has("help") || res.args.has("h")) {
            res.show_help = true;
            return res;
        }
        if (res.args.has("version") || res.args.has("V")) {
            res.show_version = true;
            return res;
        }

        // Collect any positional tokens that yargs-parser attached to pure boolean flags
        let reclaimed: [16]ParsedItem = undefined;
        let reclaimed_count: usize = 0;

        // Pass 1: resolve short->long aliases, boolean flag reclamation, negations, defaults & required options
        let oi: usize = 0;
        while (oi < _.option_count) {
            const opt = _.options[oi];
            const spec = opt.spec;
            let short_arr: [1]u8 = [spec.short_flag];
            const short_str: string = if (spec.short_flag != 0) short_arr[0..1] else "";

            const long_idx = if (spec.long_name.len > 0) res.args.findIndex(spec.long_name) else -1;
            const short_idx = if (spec.short_flag != 0) res.args.findIndex(short_str) else -1;

            // For pure boolean switches (no <arg>/[arg]), reclaim any non-flag token swallowed as value
            if (!spec.takes_arg && !spec.negate) {
                if (long_idx >= 0) {
                    const e = res.args.entries[long_idx as usize];
                    if (e.kind == ARG_STRING && e.str_len > 0) {
                        reclaimed_count = cmdPushPos(reclaimed[0..CMD_MAX_POSITIONALS], reclaimed_count, e.str_val[0..e.str_len]);
                        res.args.entries[long_idx as usize].kind = ARG_BOOL;
                        res.args.entries[long_idx as usize].bool_val = true;
                        res.args.entries[long_idx as usize].str_len = 0;
                    } else if (e.kind == ARG_INT) {
                        let ibuf: [32]u8 = undefined;
                        const ilen = cmdFormatInt(e.int_val, ibuf[0..32]);
                        reclaimed_count = cmdPushPos(reclaimed[0..CMD_MAX_POSITIONALS], reclaimed_count, ibuf[0..ilen]);
                        res.args.entries[long_idx as usize].kind = ARG_BOOL;
                        res.args.entries[long_idx as usize].bool_val = true;
                        res.args.entries[long_idx as usize].int_val = 1;
                    }
                }
                if (short_idx >= 0) {
                    const e = res.args.entries[short_idx as usize];
                    if (e.kind == ARG_STRING && e.str_len > 0) {
                        reclaimed_count = cmdPushPos(reclaimed[0..CMD_MAX_POSITIONALS], reclaimed_count, e.str_val[0..e.str_len]);
                        res.args.entries[short_idx as usize].kind = ARG_BOOL;
                        res.args.entries[short_idx as usize].bool_val = true;
                        res.args.entries[short_idx as usize].str_len = 0;
                    } else if (e.kind == ARG_INT) {
                        let ibuf: [32]u8 = undefined;
                        const ilen = cmdFormatInt(e.int_val, ibuf[0..32]);
                        reclaimed_count = cmdPushPos(reclaimed[0..CMD_MAX_POSITIONALS], reclaimed_count, ibuf[0..ilen]);
                        res.args.entries[short_idx as usize].kind = ARG_BOOL;
                        res.args.entries[short_idx as usize].bool_val = true;
                        res.args.entries[short_idx as usize].int_val = 1;
                    }
                }
            }

            // Copy short-flag value to canonical long name if long name wasn't provided directly
            if (long_idx < 0 && short_idx >= 0 && spec.long_name.len > 0) {
                const se = res.args.entries[short_idx as usize];
                cmdInsertEntry(
                    &res.args,
                    spec.long_name,
                    se.kind,
                    se.bool_val,
                    se.int_val,
                    se.str_val[0..se.str_len],
                );
            }

            const has_opt = (long_idx >= 0) || (short_idx >= 0);
            const canonical = if (spec.long_name.len > 0) spec.long_name else short_str;

            if (!has_opt) {
                if (opt.required) {
                    if (res.ok) {
                        res.ok = false;
                        let ep: usize = 0;
                        ep = cmdAppendBytes(res.error_msg[0..192], ep, "error: required option '");
                        ep = cmdAppendBytes(res.error_msg[0..192], ep, opt.flags);
                        ep = cmdAppendBytes(res.error_msg[0..192], ep, "' not specified");
                        res.error_len = ep;
                    }
                } else if (spec.negate && canonical.len > 0) {
                    // Commander `--no-foo` defaults `foo` to `true` when absent
                    cmdInsertEntry(&res.args, canonical, ARG_BOOL, true, 1, "");
                } else if (opt.has_default && canonical.len > 0) {
                    cmdSetDefaultEntry(&res.args, canonical, opt.default_str);
                }
            }

            oi = oi + 1;
        }

        // Pass 2: assemble combined positional list (reclaimed + parsed.positionals)
        let combined: [16]ParsedItem = undefined;
        let combined_count: usize = 0;

        let ri: usize = 0;
        while (ri < reclaimed_count && combined_count < CMD_MAX_POSITIONALS) {
            combined[combined_count] = reclaimed[ri];
            combined_count = combined_count + 1;
            ri = ri + 1;
        }
        let pi: usize = 0;
        while (pi < res.args.pos_count && combined_count < CMD_MAX_POSITIONALS) {
            combined[combined_count] = res.args.positionals[pi];
            combined_count = combined_count + 1;
            pi = pi + 1;
        }

        // Pass 3: subcommand routing on first positional token
        let start_pos: usize = 0;
        if (combined_count > 0 && _.subcommand_count > 0) {
            const first = combined[0];
            const first_str = first.data[0..first.len];
            let si: usize = 0;
            while (si < _.subcommand_count) {
                if (cmdStrEq(_.subcommands[si].name, first_str)) {
                    res.subcommand_len = cmdCopyBuf(res.subcommand[0..64], first_str);
                    start_pos = 1;
                    break;
                }
                si = si + 1;
            }
        }

        let ci = start_pos;
        while (ci < combined_count && res.pos_count < CMD_MAX_POSITIONALS) {
            res.positionals[res.pos_count] = combined[ci];
            res.pos_count = res.pos_count + 1;
            ci = ci + 1;
        }

        // Pass 4: validate required `<arg>` positional definitions
        if (res.ok) {
            let ai: usize = 0;
            while (ai < _.argument_count) {
                const arg_def = _.arguments[ai];
                if (arg_def.required && ai >= res.pos_count) {
                    res.ok = false;
                    let ep: usize = 0;
                    ep = cmdAppendBytes(res.error_msg[0..192], ep, "error: missing required argument '");
                    ep = cmdAppendBytes(res.error_msg[0..192], ep, arg_def.name);
                    ep = cmdAppendByte(res.error_msg[0..192], ep, 39); // '\''
                    res.error_len = ep;
                    break;
                }
                ai = ai + 1;
            }
        }

        return res;
    }

    /// Render Commander-style `--help` text via `@cap/cliui` into `out`. Returns bytes written.
    pub fn helpInformation(width: usize, out: []u8): usize {
        const w: usize = if (width == 0) 80 else width;
        let ui = CliUi.new(w);

        // 1. Usage line
        let usage_buf: [256]u8 = undefined;
        let up: usize = 0;
        up = cmdAppendBytes(usage_buf[0..256], up, "Usage: ");
        up = cmdAppendBytes(usage_buf[0..256], up, _.name);
        up = cmdAppendBytes(usage_buf[0..256], up, " [options]");
        let ai: usize = 0;
        while (ai < _.argument_count) {
            up = cmdAppendByte(usage_buf[0..256], up, 32);
            up = cmdAppendBytes(usage_buf[0..256], up, _.arguments[ai].spec);
            ai = ai + 1;
        }
        if (_.subcommand_count > 0) {
            up = cmdAppendBytes(usage_buf[0..256], up, " [command]");
        }
        let ucol: [1]Column = [Column.plain(usage_buf[0..up])];
        ui.div(&ucol);

        // 2. Description
        if (_.desc.len > 0) {
            let blank: [1]Column = [Column.plain("")];
            ui.div(&blank);
            let dcol: [1]Column = [Column.plain(_.desc)];
            ui.div(&dcol);
        }

        // Compute balanced left-column width for Arguments / Options / Commands
        let left_w: usize = 22;
        let oi: usize = 0;
        while (oi < _.option_count) {
            const need = _.options[oi].flags.len + 4;
            if (need > left_w) {
                left_w = need;
            }
            oi = oi + 1;
        }
        if (left_w > w / 2) {
            left_w = w / 2;
        }
        if (left_w < 16) {
            left_w = 16;
        }

        // 3. Arguments section
        if (_.argument_count > 0) {
            let blank: [1]Column = [Column.plain("")];
            ui.div(&blank);
            let hdr: [1]Column = [Column.plain("Arguments:")];
            ui.div(&hdr);

            let k: usize = 0;
            while (k < _.argument_count) {
                const a = _.arguments[k];
                let lbuf: [128]u8 = undefined;
                let lp: usize = 0;
                lp = cmdAppendBytes(lbuf[0..128], lp, "  ");
                lp = cmdAppendBytes(lbuf[0..128], lp, a.spec);
                let cols: [2]Column = [
                    Column.sized(lbuf[0..lp], left_w),
                    Column.plain(a.desc),
                ];
                ui.div(&cols);
                k = k + 1;
            }
        }

        // 4. Options section
        let blank_opt: [1]Column = [Column.plain("")];
        ui.div(&blank_opt);
        let opt_hdr: [1]Column = [Column.plain("Options:")];
        ui.div(&opt_hdr);

        if (_.version_str.len > 0) {
            let vcols: [2]Column = [
                Column.sized("  -V, --version", left_w),
                Column.plain("output the version number"),
            ];
            ui.div(&vcols);
        }

        let j: usize = 0;
        while (j < _.option_count) {
            const opt = _.options[j];
            let lbuf: [128]u8 = undefined;
            let lp: usize = 0;
            lp = cmdAppendBytes(lbuf[0..128], lp, "  ");
            lp = cmdAppendBytes(lbuf[0..128], lp, opt.flags);

            let rbuf: [256]u8 = undefined;
            let rp: usize = 0;
            rp = cmdAppendBytes(rbuf[0..256], rp, opt.desc);
            if (opt.has_default) {
                rp = cmdAppendBytes(rbuf[0..256], rp, " (default: \"");
                rp = cmdAppendBytes(rbuf[0..256], rp, opt.default_str);
                rp = cmdAppendBytes(rbuf[0..256], rp, "\")");
            }

            let cols: [2]Column = [
                Column.sized(lbuf[0..lp], left_w),
                Column.plain(rbuf[0..rp]),
            ];
            ui.div(&cols);
            j = j + 1;
        }

        let hcols: [2]Column = [
            Column.sized("  -h, --help", left_w),
            Column.plain("display help for command"),
        ];
        ui.div(&hcols);

        // 5. Commands section
        if (_.subcommand_count > 0) {
            let blank_cmd: [1]Column = [Column.plain("")];
            ui.div(&blank_cmd);
            let cmd_hdr: [1]Column = [Column.plain("Commands:")];
            ui.div(&cmd_hdr);

            let si: usize = 0;
            while (si < _.subcommand_count) {
                const sc = _.subcommands[si];
                let lbuf: [128]u8 = undefined;
                let lp: usize = 0;
                lp = cmdAppendBytes(lbuf[0..128], lp, "  ");
                lp = cmdAppendBytes(lbuf[0..128], lp, sc.name);

                // Reuse formatTwoCol helper for clean two-column alignment check / formatting
                let row_buf: [512]u8 = undefined;
                const row_len = formatTwoCol(lbuf[0..lp], sc.desc, left_w, w, row_buf[0..512]);
                let scol: [1]Column = [Column.plain(row_buf[0..row_len])];
                ui.div(&scol);
                si = si + 1;
            }
        }

        return ui.render(out);
    }
}

export {
    ARG_BOOL,
    ARG_INT,
    ARG_STRING,
    FlagSpec,
    OptionDef,
    ArgDef,
    SubcommandDef,
    CommandParseResult,
    Command,
    parseOptionFlags,
    parseArgSpec,
};
