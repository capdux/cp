// @cap/yargs — Zero-allocation CLI application framework for CP
// Clean-room MIT implementation with parity to npm `yargs` v17.
// Composes `@cap/yargs-parser` (argument parsing), `@cap/cliui` (multi-column help layout),
// and `@cap/y18n` (localized headings & validation messages) with zero duplication.

import {
    ARG_BOOL,
    ARG_INT,
    ARG_STRING,
    ParsedEntry,
    ParsedArgs,
    parseArgs,
} from "../../yargs-parser/src/index.cp";
import {
    Column,
    CliUi,
    formatTwoCol,
} from "../../cliui/src/index.cp";
import {
    Y18nCatalog,
} from "../../y18n/src/index.cp";

const YG_MAX_COMMANDS: usize = 16;
const YG_MAX_OPTIONS: usize = 24;

fn ygStrEq(a: string, b: string): bool {
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

fn ygAppendBytes(dst: []u8, pos: usize, s: string): usize {
    let w = pos;
    let i: usize = 0;
    while (i < s.len && w < dst.len) {
        dst[w] = s[i];
        w = w + 1;
        i = i + 1;
    }
    return w;
}

fn ygAppendByte(dst: []u8, pos: usize, b: u8): usize {
    if (pos < dst.len) {
        dst[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn ygCopyBuf(dst: []u8, src: string): usize {
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

fn ygInsertEntry(
    parsed: *ParsedArgs,
    name: string,
    kind: u8,
    bool_val: bool,
    int_val: i64,
    str_val: string,
) {
    if (parsed.entry_count >= 32 || name.len == 0) {
        return;
    }
    let key: [64]u8 = undefined;
    let camel_key: [64]u8 = undefined;
    let kebab_key: [64]u8 = undefined;
    let sbuf: [128]u8 = undefined;
    key[0] = 0;
    camel_key[0] = 0;
    kebab_key[0] = 0;
    sbuf[0] = 0;

    const klen = ygCopyBuf(key[0..64], name);
    let slen: usize = 0;
    if (kind == ARG_STRING) {
        slen = ygCopyBuf(sbuf[0..128], str_val);
    }

    parsed.entries[parsed.entry_count] = ParsedEntry {
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
    parsed.entry_count = parsed.entry_count + 1;
}

/// Option definition for `YargsApp`.
pub struct OptionSpec {
    pub name: string,
    pub alias_ch: u8,
    pub desc: string,
    pub kind: u8,
    pub required: bool,
    pub default_str: string,
    pub default_int: i64,
    pub default_bool: bool,

    /// Construct a boolean option specification.
    pub static fn boolOpt(name: string, alias_ch: u8, desc: string, default_bool: bool): OptionSpec {
        return OptionSpec {
            name: name,
            alias_ch: alias_ch,
            desc: desc,
            kind: ARG_BOOL,
            required: false,
            default_str: "",
            default_int: 0,
            default_bool: default_bool,
        };
    }

    /// Construct an integer option specification.
    pub static fn intOpt(name: string, alias_ch: u8, desc: string, default_int: i64, required: bool): OptionSpec {
        return OptionSpec {
            name: name,
            alias_ch: alias_ch,
            desc: desc,
            kind: ARG_INT,
            required: required,
            default_str: "",
            default_int: default_int,
            default_bool: false,
        };
    }

    /// Construct a string option specification.
    pub static fn strOpt(name: string, alias_ch: u8, desc: string, default_str: string, required: bool): OptionSpec {
        return OptionSpec {
            name: name,
            alias_ch: alias_ch,
            desc: desc,
            kind: ARG_STRING,
            required: required,
            default_str: default_str,
            default_int: 0,
            default_bool: false,
        };
    }
}

/// Subcommand definition for `YargsApp`.
pub struct CommandSpec {
    pub name: string,
    pub desc: string,
}

/// Result returned by `YargsApp.parse(argv)`.
pub struct YargsResult {
    pub args: ParsedArgs,
    pub command: [64]u8,
    pub command_len: usize,
    pub show_help: bool,
    pub show_version: bool,
    pub ok: bool,
    pub error_msg: [256]u8,
    pub error_len: usize,

    /// Return `true` if a registered subcommand was matched.
    pub fn hasCommand(): bool {
        return _.command_len > 0;
    }

    /// Copy the matched subcommand name into `out`. Returns bytes written (`0` if none).
    pub fn commandName(out: []u8): usize {
        if (_.command_len == 0 || out.len < _.command_len) {
            return 0;
        }
        let i: usize = 0;
        while (i < _.command_len) {
            out[i] = _.command[i];
            i = i + 1;
        }
        return _.command_len;
    }

    /// Copy the validation error message into `out`. Returns bytes written (`0` if no error).
    pub fn errorMessage(out: []u8): usize {
        if (_.error_len == 0 || out.len < _.error_len) {
            return 0;
        }
        let i: usize = 0;
        while (i < _.error_len) {
            out[i] = _.error_msg[i];
            i = i + 1;
        }
        return _.error_len;
    }

    /// Check if option `key` is present (including mapped aliases and injected defaults).
    pub fn has(key: string): bool {
        return _.args.has(key);
    }

    /// Retrieve boolean option value.
    pub fn getBool(key: string, default_val: bool): bool {
        return _.args.getBool(key, default_val);
    }

    /// Retrieve integer option value.
    pub fn getInt(key: string, default_val: i64): i64 {
        return _.args.getInt(key, default_val);
    }

    /// Retrieve string option value into `out`. Returns bytes written or `-1`.
    pub fn getString(key: string, out: []u8): i64 {
        return _.args.getString(key, out);
    }

    /// Return number of positional arguments.
    pub fn positionalCount(): usize {
        return _.args.positionalCount();
    }

    /// Copy positional argument `idx` into `out`.
    pub fn positionalAt(idx: usize, out: []u8): usize {
        return _.args.positionalAt(idx, out);
    }

    /// Return number of `--` passthrough arguments.
    pub fn restCount(): usize {
        return _.args.restCount();
    }

    /// Copy `--` passthrough argument `idx` into `out`.
    pub fn restAt(idx: usize, out: []u8): usize {
        return _.args.restAt(idx, out);
    }
}

/// Zero-allocation CLI application builder & parser (npm `yargs` v17 parity).
pub struct YargsApp {
    pub script_name: string,
    pub usage_text: string,
    pub version_str: string,
    pub commands: [16]CommandSpec,
    pub cmd_count: usize,
    pub options: [24]OptionSpec,
    pub opt_count: usize,
    pub i18n: Y18nCatalog,

    /// Create a new `YargsApp` with `script_name` and `version`.
    pub static fn new(script_name: string, version: string): YargsApp {
        let cmds: [16]CommandSpec = undefined;
        let opts: [24]OptionSpec = undefined;
        let cat = Y18nCatalog.new("en");
        cat.setEntry("Commands:", "Commands:");
        cat.setEntry("Options:", "Options:");
        cat.setEntry("Show help", "Show help");
        cat.setEntry("Show version number", "Show version number");
        cat.setEntry("Missing required argument: %s", "Missing required argument: %s");
        cat.setEntry("[required]", "[required]");
        cat.setEntry("[boolean]", "[boolean]");
        cat.setEntry("[number]", "[number]");
        cat.setEntry("[string]", "[string]");
        return YargsApp {
            script_name: script_name,
            usage_text: "",
            version_str: version,
            commands: cmds,
            cmd_count: 0,
            options: opts,
            opt_count: 0,
            i18n: cat,
        };
    }

    /// Set the CLI usage banner string.
    pub fn usage(*_, text: string) {
        _.usage_text = text;
    }

    /// Register a subcommand. Returns `false` if command table (16) is full.
    pub fn command(*_, name: string, desc: string): bool {
        if (_.cmd_count >= YG_MAX_COMMANDS) {
            return false;
        }
        _.commands[_.cmd_count] = CommandSpec {
            name: name,
            desc: desc,
        };
        _.cmd_count = _.cmd_count + 1;
        return true;
    }

    /// Register an option specification. Returns `false` if option table (24) is full.
    pub fn option(*_, spec: OptionSpec): bool {
        if (_.opt_count >= YG_MAX_OPTIONS) {
            return false;
        }
        _.options[_.opt_count] = spec;
        _.opt_count = _.opt_count + 1;
        return true;
    }

    /// Customize a localized string in the embedded `@cap/y18n` catalog.
    pub fn setTranslation(*_, key: string, translation: string): bool {
        return _.i18n.setEntry(key, translation);
    }

    /// Parse `argv`, resolve aliases & defaults, detect `--help`/`--version`,
    /// match subcommands, and validate required options.
    pub fn parse(argv: []const string): YargsResult {
        let parsed = parseArgs(argv);
        const show_help = parsed.getBool("help", false) || parsed.getBool("h", false);
        const show_version = parsed.getBool("version", false) || parsed.getBool("v", false);

        let cmdBuf: [64]u8 = undefined;
        cmdBuf[0] = 0;
        let cmdLen: usize = 0;

        let errBuf: [256]u8 = undefined;
        errBuf[0] = 0;
        let errLen: usize = 0;
        let ok = true;

        // Match first positional against registered subcommands
        if (parsed.positionalCount() > 0 && _.cmd_count > 0) {
            let pos0: [64]u8 = undefined;
            const p0Len = parsed.positionalAt(0, pos0[0..64]);
            const candidate = pos0[0..p0Len];
            let ci: usize = 0;
            while (ci < _.cmd_count) {
                if (ygStrEq(candidate, _.commands[ci].name)) {
                    cmdLen = ygCopyBuf(cmdBuf[0..64], _.commands[ci].name);
                    break;
                }
                ci = ci + 1;
            }
        }

        // Resolve aliases, inject defaults, and validate required options
        let oi: usize = 0;
        while (oi < _.opt_count) {
            const spec = _.options[oi];
            // Map short alias to canonical long option name if long name not already provided
            if (spec.alias_ch != 0 && !parsed.has(spec.name)) {
                let aliasArr: [1]u8 = [spec.alias_ch];
                const aIdx = parsed.findIndex(aliasArr[0..1]);
                if (aIdx >= 0) {
                    const srcEntry = parsed.entries[aIdx as usize];
                    ygInsertEntry(
                        &parsed,
                        spec.name,
                        srcEntry.kind,
                        srcEntry.bool_val,
                        srcEntry.int_val,
                        srcEntry.str_val[0..srcEntry.str_len],
                    );
                }
            }

            // Validate required option or inject default fallback
            if (!parsed.has(spec.name)) {
                if (spec.required) {
                    if (!show_help && !show_version && ok) {
                        ok = false;
                        const fmtArgs: [1]string = [spec.name];
                        errLen = _.i18n.t("Missing required argument: %s", fmtArgs[0..1], errBuf[0..256]);
                    }
                } else {
                    if (spec.kind == ARG_BOOL) {
                        ygInsertEntry(&parsed, spec.name, ARG_BOOL, spec.default_bool, if (spec.default_bool) 1 else 0, "");
                    } else if (spec.kind == ARG_INT) {
                        ygInsertEntry(&parsed, spec.name, ARG_INT, spec.default_int != 0, spec.default_int, "");
                    } else if (spec.default_str.len > 0) {
                        ygInsertEntry(&parsed, spec.name, ARG_STRING, true, 0, spec.default_str);
                    }
                }
            }
            oi = oi + 1;
        }

        return YargsResult {
            args: parsed,
            command: cmdBuf,
            command_len: cmdLen,
            show_help: show_help,
            show_version: show_version,
            ok: ok,
            error_msg: errBuf,
            error_len: errLen,
        };
    }

    /// Render a multi-column CLI help screen using `@cap/cliui` and `@cap/y18n` into `out`.
    /// Returns bytes written.
    pub fn renderHelp(width: usize, out: []u8): usize {
        let w = width;
        if (w < 40) {
            w = 80;
        }
        let ui = CliUi.new(w);
        const emptyArgs: [1]string = [""];

        // 1. Usage header
        if (_.usage_text.len > 0) {
            let uCols: [1]Column = [Column.plain(_.usage_text)];
            ui.div(uCols[0..1]);
        } else {
            let uBuf: [128]u8 = undefined;
            let uPos: usize = 0;
            uPos = ygAppendBytes(uBuf[0..128], uPos, _.script_name);
            if (_.cmd_count > 0) {
                uPos = ygAppendBytes(uBuf[0..128], uPos, " <command>");
            }
            uPos = ygAppendBytes(uBuf[0..128], uPos, " [options]");
            let uCols: [1]Column = [Column.plain(uBuf[0..uPos])];
            ui.div(uCols[0..1]);
        }

        // 2. Commands section
        if (_.cmd_count > 0) {
            let blank: [1]Column = [Column.plain("")];
            ui.div(blank[0..1]);

            let hdrBuf: [64]u8 = undefined;
            const hdrLen = _.i18n.t("Commands:", emptyArgs[0..0], hdrBuf[0..64]);
            let hdrCols: [1]Column = [Column.plain(hdrBuf[0..hdrLen])];
            ui.div(hdrCols[0..1]);

            let ci: usize = 0;
            while (ci < _.cmd_count) {
                let leftBuf: [96]u8 = undefined;
                let lp: usize = 0;
                lp = ygAppendBytes(leftBuf[0..96], lp, "  ");
                lp = ygAppendBytes(leftBuf[0..96], lp, _.script_name);
                lp = ygAppendByte(leftBuf[0..96], lp, 32);
                lp = ygAppendBytes(leftBuf[0..96], lp, _.commands[ci].name);

                let rowCols: [2]Column = [
                    Column.sized(leftBuf[0..lp], 28),
                    Column.plain(_.commands[ci].desc),
                ];
                ui.div(rowCols[0..2]);
                ci = ci + 1;
            }
        }

        // 3. Options section
        let blank2: [1]Column = [Column.plain("")];
        ui.div(blank2[0..1]);

        let optHdrBuf: [64]u8 = undefined;
        const optHdrLen = _.i18n.t("Options:", emptyArgs[0..0], optHdrBuf[0..64]);
        let optHdrCols: [1]Column = [Column.plain(optHdrBuf[0..optHdrLen])];
        ui.div(optHdrCols[0..1]);

        let oi: usize = 0;
        while (oi < _.opt_count) {
            const spec = _.options[oi];
            let leftBuf: [96]u8 = undefined;
            let lp: usize = 0;
            if (spec.alias_ch != 0) {
                lp = ygAppendBytes(leftBuf[0..96], lp, "  -");
                lp = ygAppendByte(leftBuf[0..96], lp, spec.alias_ch);
                lp = ygAppendBytes(leftBuf[0..96], lp, ", --");
            } else {
                lp = ygAppendBytes(leftBuf[0..96], lp, "      --");
            }
            lp = ygAppendBytes(leftBuf[0..96], lp, spec.name);

            let rightBuf: [256]u8 = undefined;
            let rp: usize = 0;
            rp = ygAppendBytes(rightBuf[0..256], rp, spec.desc);
            rp = ygAppendByte(rightBuf[0..256], rp, 32);

            let tagBuf: [32]u8 = undefined;
            let tagKey: string = "[string]";
            if (spec.kind == ARG_BOOL) {
                tagKey = "[boolean]";
            } else if (spec.kind == ARG_INT) {
                tagKey = "[number]";
            }
            const tagLen = _.i18n.t(tagKey, emptyArgs[0..0], tagBuf[0..32]);
            rp = ygAppendBytes(rightBuf[0..256], rp, tagBuf[0..tagLen]);

            if (spec.required) {
                let reqBuf: [32]u8 = undefined;
                const reqLen = _.i18n.t("[required]", emptyArgs[0..0], reqBuf[0..32]);
                rp = ygAppendByte(rightBuf[0..256], rp, 32);
                rp = ygAppendBytes(rightBuf[0..256], rp, reqBuf[0..reqLen]);
            }

            let rowCols: [2]Column = [
                Column.sized(leftBuf[0..lp], 28),
                Column.plain(rightBuf[0..rp]),
            ];
            ui.div(rowCols[0..2]);
            oi = oi + 1;
        }

        // Built-in -h, --help and -v, --version rows
        let helpDesc: [64]u8 = undefined;
        const hdLen = _.i18n.t("Show help", emptyArgs[0..0], helpDesc[0..64]);
        let helpCols: [2]Column = [
            Column.sized("  -h, --help", 28),
            Column.plain(helpDesc[0..hdLen]),
        ];
        ui.div(helpCols[0..2]);

        let verDesc: [64]u8 = undefined;
        const vdLen = _.i18n.t("Show version number", emptyArgs[0..0], verDesc[0..64]);
        let verCols: [2]Column = [
            Column.sized("  -v, --version", 28),
            Column.plain(verDesc[0..vdLen]),
        ];
        ui.div(verCols[0..2]);

        return ui.render(out);
    }
}

export {
    ARG_BOOL,
    ARG_INT,
    ARG_STRING,
    OptionSpec,
    CommandSpec,
    YargsResult,
    YargsApp,
    formatTwoCol,
};
