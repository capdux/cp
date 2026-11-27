// Type definitions for CP

import { eqIgnoreCase, toUpperByte } from "cap:ascii";
import { writeStr } from "cap:fmt";
export fn romanCharVal(b: u8): i32;
export fn toRoman(num: u16, out: []u8): usize;
export fn parseRoman(input: string): i32;
export fn isValidRoman(input: string): bool;
