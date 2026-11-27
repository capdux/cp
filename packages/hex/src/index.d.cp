// Type definitions for CP

import { hexVal, hexCharLower, hexCharUpper, isHexDigit, isPrintable } from "cap:ascii";
import { writeStr } from "cap:fmt";
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn encodeHex(input: string, out: []u8): string;
export fn encodeHexUpper(input: string, out: []u8): string;
export fn isValidHex(s: string): bool;
export fn decodeHex(hex_str: string, out: []u8): string?;
export fn decodeHexPrefixed(hex_str: string, out: []u8): string?;
export fn encodeU32Hex(v: u32, out: []u8): string;
export fn encodeU64Hex(v: u64, out: []u8): string;
export fn hexDump(data: string, out: []u8): string;
