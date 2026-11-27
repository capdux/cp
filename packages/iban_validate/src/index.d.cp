// Type definitions for CP

import { toUpperByte, isAlpha, isDigit, isAlnum } from "cap:ascii";
export fn countryLength(c0: u8, c1: u8): usize;
export fn electronicFormat(input: string, out: []u8): usize;
export fn friendlyFormat(input: string, out: []u8): usize;
export fn mod97(electronic: string): u32;
export fn isValid(input: string): bool;
export fn computeCheckDigits(country0: u8, country1: u8, bban: string, out_d0: &mut u8, out_d1: &mut u8): bool;
