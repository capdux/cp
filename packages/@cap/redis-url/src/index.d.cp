// Type definitions for CP

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

export fn ruToLower(c: u8): u8;
export fn ruEqIgnoreCase(a: &[]u8, b: &[]u8): bool;
export fn ruStartsWithIgnoreCase(s: &[]u8, prefix: &[]u8): bool;
export fn ruContainsByte(s: &[]u8, target: u8): bool;
export fn ruParseU16(s: &[]u8, out: &mut u16): bool;
export fn ruWrite(out: []u8, pos: &mut usize, src: &[]u8): bool;
export fn ruWriteU16(out: []u8, pos: &mut usize, val: u16): bool;
export fn parseRedisUrl(url: string): RedisConfig;
export fn formatRedisUrl(cfg: RedisConfig, redact_password: bool, out: []u8): usize;
