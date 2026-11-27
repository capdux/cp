// Type definitions for CP

import { TOKEN_TEXT, AnsiToken, tokenizeAnsi, stripAnsiToBuf, hasAnsi, visibleByteLen } from "../../anstyle-parse/src/index.cp";
export fn strippedLen(input: string): usize;
export fn stripAnsi(input: string, out: []u8): usize;
export fn visibleAsciiCount(input: string): usize;
