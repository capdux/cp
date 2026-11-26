// Type definitions for CP

import { hexLowerEncode, hexUpperEncode, hexDecode, DecodeResult } from "../../data-encoding/src/index.cp";
export fn encodeHex(src: &[]u8, out: []u8): usize;
export fn encodeHexUpper(src: &[]u8, out: []u8): usize;
export fn decodeHex(src: &[]u8, out: []u8): DecodeResult;
export fn hexEncodedLen(input_len: usize): usize;
export fn hexDecodedLen(hex_len: usize): usize?;
export fn isValidHex(s: string): bool;
export fn decodeHexPrefixed(src: string, out: []u8): DecodeResult;
