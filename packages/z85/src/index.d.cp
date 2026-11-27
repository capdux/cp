// Type definitions for CP

import { isSpace, findByte, trim, startsWith, endsWith } from "cap:ascii";
import { readU32BE, writeU32BE } from "cap:bin";
export fn encodeZ85(input: string, out: []u8): string?;
export fn decodeZ85(encoded: string, out: []u8): string?;
export fn encodeAscii85(input: string, out: []u8): string?;
export fn decodeAscii85(encoded: string, out: []u8): string?;
