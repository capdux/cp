// Type definitions for CP

import { hmacSha256 } from "../../cookie-signature/src/index.cp";
export fn rfcCopy32(dst: []u8, src: []u8);
export fn rfcCmpBe32(a: []u8, b: []u8): i32;
export fn rfcIsNonZero32(a: []u8): bool;
export fn rfcOrderValid32(q: []u8): bool;
export fn rfcSubBe32(a: []u8, b: []u8, out: []u8);
export class HmacDrbg256 {
    k: [32]u8,
    v: [32]u8,
    fn init(priv_key_32: []u8, msg_hash_32: []u8, extra: []u8): HmacDrbg256;
    fn nextBytes(_: auto, out: []u8): usize;
}

export fn generateK(priv_key_32: []u8, msg_hash_32: []u8, order_32: []u8, out_k32: []u8): bool;
