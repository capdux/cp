// Type definitions for CP

import { eqIgnoreCase, findByte, rfindByte } from "cap:ascii";
import { parseU16, writeStr, writeU64 } from "cap:fmt";
export class RedisConfig {
    tls: bool,
    is_unix: bool,
    host: string,
    port: u16,
    username: string,
    password: string,
    db: u16,
    socket_path: string,
    valid: bool,
}

export fn ruStartsWithIgnoreCase(s: string, prefix: string): bool;
export fn ruParseStrictU16(s: string): u16?;
export fn ruWrite(out: []u8, pos: &mut usize, src: string): bool;
export fn ruWriteU16(out: []u8, pos: &mut usize, val: u16): bool;
export fn parseRedisUrl(url: string): RedisConfig;
export fn formatRedisUrl(cfg: RedisConfig, redact_password: bool, out: []u8): usize;
