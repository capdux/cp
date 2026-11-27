// Type definitions for CP

import { HmacSha256, HMAC_SHA256_LEN } from "../../hmac/src/index.cp";
export class HkdfSha256 {
    prk: [32]u8,
    fn extract(salt: []u8, ikm: []u8): HkdfSha256;
    fn fromPrk(prk_in: []u8): HkdfSha256?;
    fn expand(info: []u8, okm: []u8): bool;
}

export fn hkdfSha256(salt: []u8, ikm: []u8, info: []u8, okm: []u8): bool;
