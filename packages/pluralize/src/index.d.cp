// Type definitions for CP

import { toUpperByte, toLower, eqIgnoreCase, endsWith, strEq } from "cap:ascii";
import { writeStr, writeI64 } from "cap:fmt";
export fn isVowelLower(c: u8): bool;
export fn detectCaseStyle(word: &[]u8): u8;
export fn copySlice(out: []u8, src: &[]u8): &[]u8;
export fn writeWithCase(out: []u8, src_lower: &[]u8, style: u8): &[]u8;
export fn writeStemAndSuffix(out: []u8, stem: &[]u8, suffix_lower: &[]u8, all_upper: bool): &[]u8;
export fn inList(lower: &[]u8, list: &[]&[]u8): bool;
export fn isUncountable(word: &[]u8): bool;
export fn plural(word: &[]u8, out: []u8): &[]u8;
export fn singular(word: &[]u8, out: []u8): &[]u8;
export fn isPlural(word: &[]u8): bool;
export fn isSingular(word: &[]u8): bool;
export fn pluralizeCount(word: &[]u8, count: i64, out: []u8): &[]u8;
export fn ordinal(n: i64, out: []u8): &[]u8;
