// @cap/shebang-command — Zero-allocation shebang executable extractor (reuses @cap/shebang-regex)
// MIT License

import { ShebangMatch, matchShebang } from "../../shebang-regex/src/index.cp";

pub struct ShebangCmd {
    pub ok: bool,
    pub bin_len: usize,
    pub arg_len: usize,
}

struct Parts {
    ok: bool,
    bin_start: usize,
    bin_len: usize,
    arg_start: usize,
    arg_len: usize,
}

fn isSpace(ch: u8): bool {
    return ch == 32 || ch == 9;
}

fn skipSpaces(s: string, pos: usize, end: usize): usize {
    let i: usize = pos;
    while (i < end && isSpace(s[i])) {
        i = i + 1;
    }
    return i;
}

fn scanTokenEnd(s: string, pos: usize, end: usize): usize {
    let i: usize = pos;
    while (i < end && !isSpace(s[i])) {
        i = i + 1;
    }
    return i;
}

fn trimRight(s: string, start: usize, end: usize): usize {
    let e: usize = end;
    while (e > start && isSpace(s[e - 1])) {
        e = e - 1;
    }
    return e;
}

fn basenameStart(s: string, start: usize, end: usize): usize {
    let base: usize = start;
    let i: usize = start;
    while (i < end) {
        if (s[i] == 47 || s[i] == 92) {
            base = i + 1;
        }
        i = i + 1;
    }
    return base;
}

fn sliceIsEnv(s: string, start: usize, len: usize): bool {
    return len == 3 && s[start] == 101 && s[start + 1] == 110 && s[start + 2] == 118;
}

fn locateParts(input: string): Parts {
    let m: ShebangMatch = matchShebang(input);
    if (!m.matched || m.cmd_len == 0) {
        return Parts { ok: false, bin_start: 0, bin_len: 0, arg_start: 0, arg_len: 0 };
    }

    let line_end: usize = trimRight(input, m.cmd_start, m.cmd_start + m.cmd_len);
    let t1_start: usize = skipSpaces(input, m.cmd_start, line_end);
    if (t1_start >= line_end) {
        return Parts { ok: false, bin_start: 0, bin_len: 0, arg_start: 0, arg_len: 0 };
    }

    let t1_end: usize = scanTokenEnd(input, t1_start, line_end);
    let b1_start: usize = basenameStart(input, t1_start, t1_end);
    let b1_len: usize = t1_end - b1_start;

    if (sliceIsEnv(input, b1_start, b1_len)) {
        // Handle `#!/usr/bin/env [-S] <cmd> [args...]`
        let cur: usize = skipSpaces(input, t1_end, line_end);
        while (cur < line_end) {
            let tok_end: usize = scanTokenEnd(input, cur, line_end);
            // Skip env flags like `-S`, `-i`
            if (input[cur] == 45) {
                cur = skipSpaces(input, tok_end, line_end);
            } else {
                let b2_start: usize = basenameStart(input, cur, tok_end);
                let b2_len: usize = tok_end - b2_start;
                let a_start: usize = skipSpaces(input, tok_end, line_end);
                let a_len: usize = 0;
                if (a_start < line_end) {
                    a_len = line_end - a_start;
                }
                return Parts {
                    ok: b2_len > 0,
                    bin_start: b2_start,
                    bin_len: b2_len,
                    arg_start: a_start,
                    arg_len: a_len,
                };
            }
        }
    }

    // Standard interpreter path: `#!/bin/bash -e` -> bin="bash", arg="-e"
    let arg_start: usize = skipSpaces(input, t1_end, line_end);
    let arg_len: usize = 0;
    if (arg_start < line_end) {
        arg_len = line_end - arg_start;
    }
    return Parts {
        ok: b1_len > 0,
        bin_start: b1_start,
        bin_len: b1_len,
        arg_start: arg_start,
        arg_len: arg_len,
    };
}

pub fn shebangCommand(input: string, out_cmd: []u8): usize {
    let p: Parts = locateParts(input);
    if (!p.ok || p.bin_len == 0 || out_cmd.len < p.bin_len) {
        return 0;
    }
    let i: usize = 0;
    while (i < p.bin_len) {
        out_cmd[i] = input[p.bin_start + i];
        i = i + 1;
    }
    return p.bin_len;
}

pub fn parseShebangCmd(input: string, out_bin: []u8, out_arg: []u8): ShebangCmd {
    let p: Parts = locateParts(input);
    if (!p.ok || p.bin_len == 0 || out_bin.len < p.bin_len) {
        return ShebangCmd { ok: false, bin_len: 0, arg_len: 0 };
    }
    let i: usize = 0;
    while (i < p.bin_len) {
        out_bin[i] = input[p.bin_start + i];
        i = i + 1;
    }
    let copied_arg: usize = 0;
    if (p.arg_len > 0 && out_arg.len >= p.arg_len) {
        let j: usize = 0;
        while (j < p.arg_len) {
            out_arg[j] = input[p.arg_start + j];
            j = j + 1;
        }
        copied_arg = p.arg_len;
    }
    return ShebangCmd {
        ok: true,
        bin_len: p.bin_len,
        arg_len: copied_arg,
    };
}
