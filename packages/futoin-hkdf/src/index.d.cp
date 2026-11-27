// Type definitions for CP

import { hmacSha256, sha256Init, sha256Update, sha256Final } from "../../cookie-signature/src/index.cp";
export fn hkdfStreamingHmac(prk32: []u8, prev_t: []u8, info: []u8, counter: u8, out32: []u8);
export fn hkdfExtract(salt: []u8, ikm: []u8, out_prk32: []u8): bool;
export fn hkdfExpand(prk32: []u8, info: []u8, out_okm: []u8): bool;
export fn hkdfDerive(salt: []u8, ikm: []u8, info: []u8, out_okm: []u8): bool;
