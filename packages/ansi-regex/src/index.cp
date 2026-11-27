// @cap/ansi-regex — Zero-heap ANSI escape sequence matcher & span finder
// Directly reuses @cap/anstyle-parse (crates.io #151) for unified zero-duplication VT100 parsing!
// Matching npm ansi-regex (400M+/wk). Clean-room MIT in pure CP.

import {
    TOKEN_TEXT, AnsiToken, tokenizeAnsi, hasAnsi, stripAnsiToBuf
} from "../../anstyle-parse/src/index.cp";

pub struct AnsiSpan {
    pub start: usize,
    pub len: usize,
    pub found: bool,
}

pub fn testAnsi(input: string): bool {
    return hasAnsi(input);
}

pub fn findFirstAnsi(input: string): AnsiSpan {
    let toks: [64]AnsiToken = [AnsiToken { kind: 0, start: 0, len: 0, final_byte: 0 }; 64];
    let n = tokenizeAnsi(input, toks[0..64]);
    let i: usize = 0;
    while (i < n) {
        if (toks[i].kind != TOKEN_TEXT) {
            return AnsiSpan { start: toks[i].start, len: toks[i].len, found: true };
        }
        i = i + 1;
    }
    return AnsiSpan { start: 0, len: 0, found: false };
}

pub fn findAllAnsi(input: string, out: []AnsiSpan): usize {
    let toks: [128]AnsiToken = [AnsiToken { kind: 0, start: 0, len: 0, final_byte: 0 }; 128];
    let n = tokenizeAnsi(input, toks[0..128]);
    let count: usize = 0;
    let i: usize = 0;
    while (i < n && count < out.len) {
        if (toks[i].kind != TOKEN_TEXT) {
            out[count] = AnsiSpan { start: toks[i].start, len: toks[i].len, found: true };
            count = count + 1;
        }
        i = i + 1;
    }
    return count;
}

pub fn countAnsiSequences(input: string): usize {
    let toks: [128]AnsiToken = [AnsiToken { kind: 0, start: 0, len: 0, final_byte: 0 }; 128];
    let n = tokenizeAnsi(input, toks[0..128]);
    let count: usize = 0;
    let i: usize = 0;
    while (i < n) {
        if (toks[i].kind != TOKEN_TEXT) {
            count = count + 1;
        }
        i = i + 1;
    }
    return count;
}

pub fn stripAllAnsi(input: string, out: []u8): usize {
    return stripAnsiToBuf(input, out);
}
