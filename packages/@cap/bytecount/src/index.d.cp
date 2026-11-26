// Type definitions for CP

import { countByte } from "../../memchr-crate/src/index.cp";
export fn count(haystack: []u8, needle: u8): usize;
export fn countFast(haystack: []u8, needle: u8): usize;
export fn numChars(utf8_bytes: []u8): usize;
export fn countLines(buf: []u8): usize;
export fn countAsciiWhitespace(buf: []u8): usize;
export fn countInRange(buf: []u8, lo: u8, hi: u8): usize;
