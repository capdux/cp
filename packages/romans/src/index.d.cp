// Type definitions for CP

import { eqIgnoreCase, toUpperByte, toLowerByte } from "cap:ascii";
import { writeStr } from "cap:fmt";

export const MIN_ROMAN: u16;
export const MAX_ROMAN: u16;
export const MAX_ROMAN_LEN: usize;

export fn romanCharValue(b: u8): i32;
export fn romanLen(num: u16): usize;
export fn toRoman(num: u16, out: []u8): usize;
export fn toRomanLower(num: u16, out: []u8): usize;
export fn romanize(num: i32, out: []u8): usize;
export fn romanizeChecked(num: i32, out: []u8): usize?;
export fn parseRoman(input: string): i32;
export fn deromanize(input: string): i32?;
export fn isValidRoman(input: string): bool;
