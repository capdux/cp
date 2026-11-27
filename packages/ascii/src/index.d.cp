// Type definitions for CP

import { isAscii, firstNonAscii, eqIgnoreCase, toLower, toUpper, trim } from "cap:ascii";

export fn isAsciiAlpha(b: u8): bool;
export fn isAsciiDigit(b: u8): bool;
export fn isAsciiAlphanumeric(b: u8): bool;
export fn isAsciiHexDigit(b: u8): bool;
export fn isAsciiOctDigit(b: u8): bool;
export fn isAsciiWhitespace(b: u8): bool;
export fn isAsciiPrintable(b: u8): bool;
export fn isAsciiControl(b: u8): bool;
export fn isAsciiLowercase(b: u8): bool;
export fn isAsciiUppercase(b: u8): bool;
export fn isAsciiGraph(b: u8): bool;
export fn isAsciiPunctuation(b: u8): bool;
export fn toAsciiLowerByte(b: u8): u8;
export fn toAsciiUpperByte(b: u8): u8;

export class AsciiChar {
    byte: u8,
    static fn fromByte(b: u8): AsciiChar?;
    fn asByte(): u8;
    fn isAlphabetic(): bool;
    fn isAlphanumeric(): bool;
    fn isControl(): bool;
    fn isDigit(): bool;
    fn isGraph(): bool;
    fn isHexDigit(): bool;
    fn isLowercase(): bool;
    fn isPrintable(): bool;
    fn isPunctuation(): bool;
    fn isUppercase(): bool;
    fn isWhitespace(): bool;
    fn toAsciiUppercase(): AsciiChar;
    fn toAsciiLowercase(): AsciiChar;
    fn eqIgnoreAsciiCase(other: AsciiChar): bool;
}

export class AsciiStr {
    bytes: string,
    static fn fromStr(s: string): AsciiStr?;
    fn len(): usize;
    fn isEmpty(): bool;
    fn asStr(): string;
    fn get(idx: usize): AsciiChar?;
    fn eqIgnoreAsciiCase(other: AsciiStr): bool;
    fn trim(): AsciiStr;
    fn trimStart(): AsciiStr;
    fn trimEnd(): AsciiStr;
    fn toAsciiLowercase(out: []u8): usize;
    fn toAsciiUppercase(out: []u8): usize;
}

export fn eqIgnoreAsciiCase(a: string, b: string): bool;
export fn toAsciiLower(s: string, out: []u8): usize;
export fn toAsciiUpper(s: string, out: []u8): usize;
export fn toAsciiLowercase(s: string, out: []u8): usize;
export fn toAsciiUppercase(s: string, out: []u8): usize;
export fn makeAsciiLowercase(buf: []u8);
export fn makeAsciiUppercase(buf: []u8);
export fn trimAscii(s: string): string;
