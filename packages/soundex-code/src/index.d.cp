// Type definitions for CP

import { isAlpha, toUpperByte, strEq } from "cap:ascii";

export const DEFAULT_SOUNDEX_LEN: usize;

export fn soundexCharCode(ch: u8): u8;
export fn soundex(word: string, out: []u8): usize;
export fn soundexCode(word: string, max_length: usize, out: []u8): usize;
export fn soundexPadded(word: string, length: usize, out: []u8): usize;
export fn soundsLike(a: string, b: string): bool;
export fn soundsLikeWithLength(a: string, b: string, max_length: usize): bool;
export fn soundexDifference(a: string, b: string): u8;
