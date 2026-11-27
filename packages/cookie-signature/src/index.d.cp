// Type definitions for CP

export class CookieSlice {
    start: usize,
    len: usize,
    found: bool,
}

export class CookieOptions {
    max_age: i64,
    path: &[]u8,
    domain: &[]u8,
    http_only: bool,
    secure: bool,
    same_site: u8,
}

export fn add32(a: u32, b: u32): u32;
export fn rotr32(x: u32, n: u32): u32;
export fn sha256K(i: usize): u32;
export class UnsignResult {
    ok: bool,
    val_len: usize,
}

export class Sha256State {
    h: [8]u32,
    block: [64]u8,
    block_len: usize,
    total_bytes: u64,
    fn init(): Sha256State;
    fn compress(_: auto, blk: &[]u8);
    fn update(_: auto, data: &[]u8);
    fn finish(_: auto, out: []u8);
}

export fn sha256Init(): Sha256State;
export fn sha256Update(st: &mut Sha256State, data: &[]u8);
export fn sha256Final(st: &mut Sha256State, out32: []u8);
export fn sha256(data: &[]u8, out32: []u8);
export fn ctEqual(a: &[]u8, b: &[]u8): bool;
export fn hmacSha256(key: &[]u8, msg: &[]u8, out: []u8);
export fn b64UrlChar(v: u8): u8;
export fn encodeBase64Url32(mac: &[]u8, dst: []u8);
export fn isWs(c: u8): bool;
export fn csBytesEq(a: &[]u8, b: &[]u8): bool;
export fn getCookie(header: &[]u8, name: &[]u8): CookieSlice;
export fn sign(val: &[]u8, secret: &[]u8, dst: []u8): usize;
export fn unsign(signed_val: &[]u8, secret: &[]u8): UnsignResult;
export fn appendBytes(dst: []u8, pos: usize, src: &[]u8): i64;
export fn appendU64(dst: []u8, pos: usize, val: u64): i64;
export fn formatSetCookie(name: &[]u8, value: &[]u8, opts: CookieOptions, dst: []u8): i64;
