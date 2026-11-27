// Type definitions for CP

import { isAlpha, isDigit, toLowerByte, toUpperByte, eqIgnoreCase } from "cap:ascii";
export fn isUpper(b: u8): bool;
export fn isLower(b: u8): bool;
export fn isSmallWord(word: string): bool;
export fn isWordChar(b: u8): bool;
export fn isAllCapsAcronym(word: string): bool;
export fn hasMoreWords(input: string, start_pos: usize): bool;
export fn toTitleCase(input: string, out: []u8): usize;
export fn toSentenceCase(input: string, out: []u8): usize;
export fn capitalizeFirst(input: string, out: []u8): usize;
