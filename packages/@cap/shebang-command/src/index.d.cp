// Type definitions for CP

import { ShebangMatch, matchShebang } from "../../shebang-regex/src/index.cp";
export class ShebangCmd {
    ok: bool,
    bin_len: usize,
    arg_len: usize,
}

export class Parts {
    ok: bool,
    bin_start: usize,
    bin_len: usize,
    arg_start: usize,
    arg_len: usize,
}

export fn isSpace(ch: u8): bool;
export fn skipSpaces(s: string, pos: usize, end: usize): usize;
export fn scanTokenEnd(s: string, pos: usize, end: usize): usize;
export fn trimRight(s: string, start: usize, end: usize): usize;
export fn basenameStart(s: string, start: usize, end: usize): usize;
export fn sliceIsEnv(s: string, start: usize, len: usize): bool;
export fn locateParts(input: string): Parts;
export fn shebangCommand(input: string, out_cmd: []u8): usize;
export fn parseShebangCmd(input: string, out_bin: []u8, out_arg: []u8): ShebangCmd;
