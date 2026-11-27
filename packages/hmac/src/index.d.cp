// Type definitions for CP

import { Sha256State, sha256Init, sha256Update, sha256Final, sha256, ctEqual } from "../../cookie-signature/src/index.cp";
import { hexCharLower } from "cap:ascii";

export const HMAC_SHA256_LEN: usize;
export const HMAC_BLOCK_SIZE: usize;
export const HMAC_SHA512_LEN: usize;
export const HMAC_SHA512_BLOCK_SIZE: usize;

export fn ctEqualBytes(a: []const u8, b: []const u8): bool;
export fn encodeHex32(digest: []u8, out64: []u8): usize;
export fn encodeHex64(digest: []const u8, out128: []u8): usize;

export class HmacSha256 {
    inner: Sha256State,
    o_key_pad: [64]u8,
    fn newFromSlice(key: []u8): HmacSha256;
    fn newFromConstSlice(key: []const u8): HmacSha256;
    fn newFromKey(key: string): HmacSha256;
    fn update(_: auto, data: []u8);
    fn updateConst(_: auto, data: []const u8);
    fn updateStr(_: auto, msg: string);
    fn finalize(_: auto, out32: []u8);
    fn verifySlice(_: auto, expected: []u8): bool;
    fn verifyConstSlice(_: auto, expected: []const u8): bool;
}

export class HmacSha512 {
    fn newFromSlice(key: []const u8): HmacSha512;
    fn newFromKey(key: string): HmacSha512;
    fn update(_: auto, data: []const u8);
    fn updateStr(_: auto, msg: string);
    fn finalize(_: auto, out64: []u8);
    fn verifySlice(_: auto, expected: []const u8): bool;
}

export fn hmacSha256Bytes(key: []const u8, msg: []const u8, out32: []u8);
export fn hmacSha256Hex(key: string, msg: string, out64: []u8): usize;
export fn hmacSha512(key: []const u8, msg: []const u8, out64: []u8);
export fn hmacSha512Hex(key: string, msg: string, out128: []u8): usize;
