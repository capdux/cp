// Type definitions for CP

import { bytesEq, findByteInBytes } from "cap:bin";
import { writeStr, writeI64, parseI64 } from "cap:fmt";
import { ARG_BOOL, ARG_INT, ARG_STRING, ParsedItem, ParsedEntry, ParsedArgs, parseArgs } from "../../yargs-parser/src/index.cp";
import { Column, CliUi, formatTwoCol } from "../../cliui/src/index.cp";
export fn cmdAppendByte(dst: []u8, pos: usize, b: u8): usize;
export fn cmdIsFlagSep(ch: u8): bool;
export fn cmdTrim(s: string): string;
export fn cmdEmptyItem(): ParsedItem;
export fn cmdMakeItem(s: string): ParsedItem;
export fn cmdInsertEntry(parsed: &mut ParsedArgs, name: string, kind: u8, bool_val: bool, int_val: i64, str_val: string);
export fn cmdSetDefaultEntry(parsed: &mut ParsedArgs, name: string, default_str: string);
export class FlagSpec {
    short_flag: u8,
    long_name: string,
    arg_name: string,
    takes_arg: bool,
    arg_required: bool,
    arg_optional: bool,
    negate: bool,
    variadic: bool,
}

export fn parseOptionFlags(flags: string): FlagSpec;
export class OptionDef {
    flags: string,
    desc: string,
    default_str: string,
    has_default: bool,
    required: bool,
    spec: FlagSpec,
}

export class ArgDef {
    spec: string,
    name: string,
    desc: string,
    required: bool,
    variadic: bool,
}

export fn parseArgSpec(spec: string, desc: string): ArgDef;
export class SubcommandDef {
    name: string,
    desc: string,
}

export class CommandParseResult {
    ok: bool,
    show_help: bool,
    show_version: bool,
    args: ParsedArgs,
    subcommand: [64]u8,
    subcommand_len: usize,
    positionals: [16]ParsedItem,
    pos_count: usize,
    error_msg: [192]u8,
    error_len: usize,
    fn hasSubcommand(): bool;
    fn subcommandName(out: []u8): usize;
    fn errorMessage(out: []u8): usize;
    fn hasOption(name: string): bool;
    fn getBool(name: string, default_val: bool): bool;
    fn getInt(name: string, default_val: i64): i64;
    fn getString(name: string, out: []u8): i64;
    fn argCount(): usize;
    fn argAt(idx: usize, out: []u8): usize;
}

export fn cmdPushPos(pos_arr: []ParsedItem, count: usize, s: string): usize;
export class Command {
    name: string,
    version_str: string,
    desc: string,
    options: [16]OptionDef,
    option_count: usize,
    subcommands: [16]SubcommandDef,
    subcommand_count: usize,
    arguments: [8]ArgDef,
    argument_count: usize,
    fn new(name: string): Command;
    fn version(_: auto, v: string);
    fn description(_: auto, d: string);
    fn option(_: auto, flags: string, desc: string, default_str: string): bool;
    fn requiredOption(_: auto, flags: string, desc: string): bool;
    fn argument(_: auto, spec: string, desc: string): bool;
    fn subcommand(_: auto, name: string, desc: string): bool;
    fn parse(argv: &[]string): CommandParseResult;
    fn helpInformation(width: usize, out: []u8): usize;
}

