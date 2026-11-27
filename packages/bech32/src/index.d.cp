// Type definitions for CP

import { toLowerByte, toLower } from "cap:ascii";
import { findByteInBytes } from "cap:bin";
export enum Bech32Variant {
    Bech32 = 0,
    Bech32m = 1
}

export class Bech32Decoded {
    valid: bool,
    variant: Bech32Variant,
    hrp: string,
    data: string,
    fn invalid(): Bech32Decoded;
}

export fn charsetIndex(c: u8): u8?;
export fn polymodStep(chk: u32, v: u8): u32;
export fn polymodHrp(hrp: string): u32;
export fn variantConst(v: Bech32Variant): u32;
export fn isValidHrp(hrp: string): bool;
export fn to5BitWords(bytes: string, out: []u8): usize?;
export fn from5BitWords(words: &[]u8, out: []u8): usize?;
export fn encodeBech32(hrp: string, payload_bytes: string, variant: Bech32Variant, out: []u8): string?;
export class ParsedBech32Meta {
    valid: bool,
    variant: Bech32Variant,
    sep: usize,
}

export fn validateStructureAndChecksum(encoded: string): ParsedBech32Meta;
export fn verifyBech32(encoded: string): bool;
export fn decodeBech32(encoded: string, hrp_out: []u8, data_out: []u8): Bech32Decoded;
