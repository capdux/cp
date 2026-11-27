// Type definitions for CP

import { Sha256State, sha256Init, sha256Update, sha256Final, sha256, ctEqual } from "../../cookie-signature/src/index.cp";
export class Pbkdf2Pads {
    inner_base: Sha256State,
    outer_base: Sha256State,
}

export fn pbkdf2InitPads(password: &[]u8): Pbkdf2Pads;
export fn pbkdf2HmacSaltBlock(pads: Pbkdf2Pads, salt: &[]u8, block_idx: u32, out32: []u8);
export fn pbkdf2Hmac32(pads: Pbkdf2Pads, msg32: []u8, out32: []u8);
export fn pbkdf2HmacSha256(password: &[]u8, salt: &[]u8, iterations: u32, out_dk: []u8): bool;
export fn pbkdf2Verify(password: &[]u8, salt: &[]u8, iterations: u32, expected_dk: &[]u8): bool;
export fn pbkdf2HmacSha256Str(password: string, salt: string, iterations: u32, out_dk: []u8): bool;
