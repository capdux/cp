// Type definitions for CP

import { isAlpha, toUpperByte } from "cap:ascii";
import { bytesEq } from "cap:bin";
export fn isVowel(b: u8): bool;
export fn metaphone(word: string, max_len: usize, out: []u8): usize;
export fn soundsLike(a: string, b: string): bool;
