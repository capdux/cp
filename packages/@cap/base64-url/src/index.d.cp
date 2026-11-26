// Type definitions for CP

import { base64Encode, base64Decode, base64EncodeLen, DecodeResult } from "../../data-encoding/src/index.cp";
export fn encodeBase64Url(src: &[]u8, out: []u8): usize;
export fn decodeBase64Url(src: &[]u8, out: []u8): DecodeResult;
export fn encodedBase64UrlLen(input_len: usize): usize;
export fn fromStandardBase64(b64: string, out: []u8): usize;
export fn toStandardBase64(b64url: string, out: []u8): usize;
export fn isValidBase64Url(s: string): bool;
