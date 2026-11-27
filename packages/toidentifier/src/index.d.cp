// Type definitions for CP

import { isAlpha, isAlnum, isSpace, toUpperByte, toLowerByte } from "cap:ascii";
export fn tidIsIdentChar(c: u8): bool;
export fn toIdentifier(input: string, out: []u8): usize;
export fn toCamelIdentifier(input: string, out: []u8): usize;
export fn toConstIdentifier(input: string, out: []u8): usize;
export fn isValidIdentifier(s: string): bool;
