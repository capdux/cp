// Type definitions for CP

import { isAlnum, toUpperByte, toLowerByte } from "cap:ascii";
export fn titIsWordChar(c: u8): bool;
export fn upperCaseFirst(input: string, out: []u8): usize;
export fn titleize(input: string, out: []u8): usize;
