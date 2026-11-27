// Type definitions for CP

import { toUpperByte, isSpace, strEq } from "cap:ascii";
import { writeStr } from "cap:fmt";
export fn encodeChar(c: u8): string;
export fn decodeSymbol(sym: string): u8;
export fn encodeText(input: string, out: []u8): usize;
export fn decodeText(morse: string, out: []u8): usize;
