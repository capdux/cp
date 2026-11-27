// Type definitions for CP

import { toLowerByte } from "cap:ascii";
export fn decodeChar(b: u8): i32;
export fn encodedLen(byte_len: usize): usize;
export fn encode(data: &[]u8, out: []u8): usize;
export fn decode(input: string, out: []u8): i64;
export fn isValidZBase32(input: string): bool;
