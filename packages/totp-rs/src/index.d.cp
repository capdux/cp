// Type definitions for CP

import { eqIgnoreCase, isAlnum, hexCharUpper, findByte } from "cap:ascii";
import { parseU64, writeStr, writeU64 } from "cap:fmt";
export class OtpAuthUri {
    kind: u8,
    label: string,
    secret: string,
    issuer: string,
    algorithm: u8,
    digits: u8,
    period: u32,
    counter: u64,
    valid: bool,
}

export fn otpIsUnreserved(c: u8): bool;
export fn otpParseStrictU64(s: string): u64?;
export fn otpWriteRaw(out: []u8, pos: &mut usize, src: string): bool;
export fn otpWritePctEncoded(out: []u8, pos: &mut usize, src: string, allow_label_chars: bool): bool;
export fn otpWriteU64(out: []u8, pos: &mut usize, val: u64): bool;
export fn isValidBase32Secret(secret: string): bool;
export fn parseOtpAuthUri(uri: string): OtpAuthUri;
export fn formatOtpAuthUri(cfg: OtpAuthUri, out: []u8): usize;
export class LabelParts {
    issuer: string,
    account: string,
}

export fn splitLabel(label: string): LabelParts;
