// Type definitions for CP

import { isSpace } from "cap:ascii";
import { findByteInBytes } from "cap:bin";
export class StripOptions {
    preserve_whitespace: bool,
    strip_trailing_commas: bool,
}

export fn isTrailingComma(src: &[]u8, commaIdx: usize): bool;
export fn stripComments(src: &[]u8, dst: []u8, opts: StripOptions): i64;
export fn stripJsonc(src: &[]u8, dst: []u8): i64;
export fn stripCompact(src: &[]u8, dst: []u8): i64;
