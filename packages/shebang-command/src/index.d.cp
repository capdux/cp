// Type definitions for CP

import { isSpace, strEq } from "cap:ascii";
import { writeStr } from "cap:fmt";
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

export fn skipSpaces(s: string, pos: usize, end: usize): usize;
export fn scanTokenEnd(s: string, pos: usize, end: usize): usize;
export fn trimRight(s: string, start: usize, end: usize): usize;
export fn basenameStart(s: string, start: usize, end: usize): usize;
export fn locateParts(input: string): Parts;
export fn shebangCommand(input: string, out_cmd: []u8): usize;
export fn parseShebangCmd(input: string, out_bin: []u8, out_arg: []u8): ShebangCmd;
