// Type definitions for CP

import { hexCharUpper, hexVal } from "cap:ascii";
export fn isDirectPrintable(b: u8): bool;
export fn encode(data: &[]u8, out: []u8): usize;
export fn decode(input: string, out: []u8): i64;
