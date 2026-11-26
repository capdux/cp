// Type definitions for CP

import { ARG_BOOL, ARG_INT, ARG_STRING, ParsedEntry, ParsedArgs, parseArgs } from "../../yargs-parser/src/index.cp";
import { Column, CliUi, formatTwoCol } from "../../cliui/src/index.cp";
import { Y18nCatalog } from "../../y18n/src/index.cp";
export fn ygStrEq(a: string, b: string): bool;
export fn ygAppendBytes(dst: []u8, pos: usize, s: string): usize;
export fn ygAppendByte(dst: []u8, pos: usize, b: u8): usize;
export fn ygCopyBuf(dst: []u8, src: string): usize;
export fn ygInsertEntry(parsed: &mut ParsedArgs, name: string, kind: u8, bool_val: bool, int_val: i64, str_val: string);
export class OptionSpec {
    name: string,
    alias_ch: u8,
    desc: string,
    kind: u8,
    required: bool,
    default_str: string,
    default_int: i64,
    default_bool: bool,
    fn boolOpt(name: string, alias_ch: u8, desc: string, default_bool: bool): OptionSpec;
    fn intOpt(name: string, alias_ch: u8, desc: string, default_int: i64, required: bool): OptionSpec;
    fn strOpt(name: string, alias_ch: u8, desc: string, default_str: string, required: bool): OptionSpec;
}

export class CommandSpec {
    name: string,
    desc: string,
}

export class YargsResult {
    args: ParsedArgs,
    command: [64]u8,
    command_len: usize,
    show_help: bool,
    show_version: bool,
    ok: bool,
    error_msg: [256]u8,
    error_len: usize,
    fn hasCommand(): bool;
    fn commandName(out: []u8): usize;
    fn errorMessage(out: []u8): usize;
    fn has(key: string): bool;
    fn getBool(key: string, default_val: bool): bool;
    fn getInt(key: string, default_val: i64): i64;
    fn getString(key: string, out: []u8): i64;
    fn positionalCount(): usize;
    fn positionalAt(idx: usize, out: []u8): usize;
    fn restCount(): usize;
    fn restAt(idx: usize, out: []u8): usize;
}

export class YargsApp {
    script_name: string,
    usage_text: string,
    version_str: string,
    commands: [16]CommandSpec,
    cmd_count: usize,
    options: [24]OptionSpec,
    opt_count: usize,
    i18n: Y18nCatalog,
    fn new(script_name: string, version: string): YargsApp;
    fn usage(_: auto, text: string);
    fn command(_: auto, name: string, desc: string): bool;
    fn option(_: auto, spec: OptionSpec): bool;
    fn setTranslation(_: auto, key: string, translation: string): bool;
    fn parse(argv: &[]string): YargsResult;
    fn renderHelp(width: usize, out: []u8): usize;
}

