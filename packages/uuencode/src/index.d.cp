// Type definitions for CP

import { strEq, startsWith, findByte } from "cap:ascii";
export fn uuEncChar(v: u8): u8;
export fn uuDecVal(c: u8): i32;
export fn encodeLine(data: &[]u8, out: []u8): usize;
export fn decodeLine(line: string, out: []u8): i64;
export fn encode(data: &[]u8, out: []u8): usize;
export fn decode(input: string, out: []u8): i64;
