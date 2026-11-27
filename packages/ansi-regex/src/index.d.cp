// Type definitions for CP

import { TOKEN_TEXT, AnsiToken, tokenizeAnsi, hasAnsi, stripAnsiToBuf } from "../../anstyle-parse/src/index.cp";
export class AnsiSpan {
    start: usize,
    len: usize,
    found: bool,
}

export fn testAnsi(input: string): bool;
export fn findFirstAnsi(input: string): AnsiSpan;
export fn findAllAnsi(input: string, out: []AnsiSpan): usize;
export fn countAnsiSequences(input: string): usize;
export fn stripAllAnsi(input: string, out: []u8): usize;
