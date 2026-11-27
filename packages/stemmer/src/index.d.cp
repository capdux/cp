// Type definitions for CP

import { toLower } from "cap:ascii";
import { bytesEq, endsWithBytes } from "cap:bin";
import { writeStr } from "cap:fmt";
export fn isConsonant(buf: &[]u8, idx: usize): bool;
export fn measurePrefix(buf: &[]u8, end: usize): usize;
export fn measure(word: &[]u8): usize;
export fn containsVowel(buf: &[]u8, end: usize): bool;
export fn endsDoubleConsonant(buf: &[]u8, end: usize): bool;
export fn endsCvc(buf: &[]u8, end: usize): bool;
export fn tryStep2(buf: []u8, len: usize): usize;
export fn tryStep3(buf: []u8, len: usize): usize;
export fn tryStep4(buf: []u8, len: usize): usize;
export fn stem(word: string, out: []u8): usize;
export fn sameStem(a: string, b: string): bool;
