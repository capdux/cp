// Type definitions for CP

import { isAscii, firstNonAscii, eqIgnoreCase, toLower, toUpper, trim } from "cap:ascii";
export fn isAsciiAlpha(b: u8): bool;
export fn isAsciiDigit(b: u8): bool;
export fn isAsciiAlphanumeric(b: u8): bool;
export fn isAsciiHexDigit(b: u8): bool;
export fn isAsciiWhitespace(b: u8): bool;
export fn isAsciiPrintable(b: u8): bool;
export fn isAsciiControl(b: u8): bool;
export fn toAsciiLowerByte(b: u8): u8;
export fn toAsciiUpperByte(b: u8): u8;
export fn eqIgnoreAsciiCase(a: string, b: string): bool;
export fn toAsciiLower(s: string, out: []u8): usize;
export fn toAsciiUpper(s: string, out: []u8): usize;
export fn trimAscii(s: string): string;
