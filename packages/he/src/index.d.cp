// Type definitions for CP

import { strEq, hexVal, hexCharUpper, isDigit } from "cap:ascii";
import { writeStr } from "cap:fmt";
import { decodeCodepoint, encodeCodepointLossy } from "cap:utf8";
export fn pushByte(out: []u8, pos: usize, b: u8): usize;
export fn writeUtf8(out: []u8, pos: usize, raw_cp: u32): usize;
export fn pushHexEntity(out: []u8, pos: usize, cp: u32): usize;
export fn needsHtmlEscape(s: string): bool;
export fn escapeHtml(input: string, out: []u8): string;
export fn encodeNonAscii(input: string, out: []u8): string;
export fn lookupNamedEntity(name: string): u32;
export fn decodeHtmlEntities(input: string, out: []u8): string;
