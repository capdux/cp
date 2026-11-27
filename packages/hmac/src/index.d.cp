// Type definitions for CP

import { Sha256State, sha256Init, sha256Update, sha256Final, sha256, ctEqual } from "../../cookie-signature/src/index.cp";
import { hexCharLower } from "cap:ascii";
export fn encodeHex32(digest: []u8, out64: []u8): usize;
export class HmacSha256 {
    inner: Sha256State,
    o_key_pad: [64]u8,
    fn newFromSlice(key: []u8): HmacSha256;
    fn newFromKey(key: string): HmacSha256;
    fn update(_: auto, data: []u8);
    fn updateStr(_: auto, msg: string);
    fn finalize(_: auto, out32: []u8);
    fn verifySlice(_: auto, expected: []u8): bool;
}

export fn hmacSha256Hex(key: string, msg: string, out64: []u8): usize;
