// Type definitions for CP

import { isDigit, eqIgnoreCase, findByte } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class TelUri {
    number: string,
    is_global: bool,
    ext: string,
    phone_context: string,
    valid: bool,
}

export fn isVisualSep(c: u8): bool;
export fn isWs(c: u8): bool;
export fn isValidE164(number: string): bool;
export fn normalizeE164(input: string, out: []u8): usize;
export fn parseTelUri(uri: string): TelUri;
export fn appendSlice(out: []u8, pos: usize, s: string): i64;
export fn formatTelUri(e164_number: string, ext: string, out: []u8): usize;
