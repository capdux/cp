// Type definitions for CP

import { base64Encode, base64Decode, base64EncodeLen, DecodeResult } from "../../data-encoding/src/index.cp";
import { isAlnum } from "cap:ascii";

export fn encodeBase64Url(src: &[]u8, out: []u8): usize;
export fn encode(src: &[]u8, out: []u8): usize;
export fn encodePadded(src: &[]u8, out: []u8): usize;
export fn decodeBase64Url(src: &[]u8, out: []u8): DecodeResult;
export fn decode(src: &[]u8, out: []u8): DecodeResult;
export fn encodedBase64UrlLen(input_len: usize): usize;
export fn encodedLen(input_len: usize): usize;
export fn encodedPaddedLen(input_len: usize): usize;
export fn decodedLen(src: string): usize?;
export fn fromStandardBase64(b64: string, out: []u8): usize;
export fn fromBase64(b64: string, out: []u8): usize;
export fn toStandardBase64(b64url: string, out: []u8): usize;
export fn toBase64(b64url: string, out: []u8): usize;
export fn padString(s: string, out: []u8): usize;
export fn unpadString(s: string): string;
export fn isValidBase64Url(s: string): bool;
export fn validate(s: string): bool;
export fn validatePadded(s: string): bool;
