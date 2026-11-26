// Type definitions for CP

import { HmacSha256, HMAC_SHA256_LEN } from "../../hmac-crate/src/index.cp";
export fn pbkdf2HmacSha256(password: []u8, salt: []u8, iterations: u32, dk_out: []u8): bool;
export fn pbkdf2HmacSha256Str(password: string, salt: string, iterations: u32, dk_out: []u8): bool;
