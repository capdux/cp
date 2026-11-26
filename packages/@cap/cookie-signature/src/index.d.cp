// Type definitions for CP

import { encodeStdUnpadded } from "../../base64ct/src/index.cp";
export fn add32(a: u32, b: u32): u32;
export fn rotr32(x: u32, n: u32): u32;
export class Sha256State {
    h: [8]u32,
    buf: [64]u8,
    buf_len: usize,
    total_bytes: u64,
}

export fn sha256Init(): Sha256State;
export fn sha256Compress(st: &mut Sha256State, block: &[]u8);
export fn sha256Update(st: &mut Sha256State, data: &[]u8);
export fn sha256Final(st: &mut Sha256State, out32: []u8);
export fn sha256(data: &[]u8, out32: []u8);
export fn hmacSha256(key: string, msg: string, out32: []u8);
export fn ctEqual(a: &[]u8, b: &[]u8): bool;
export class UnsignResult {
    ok: bool,
    val_len: usize,
}

export fn sign(val: string, secret: string, out: []u8): usize;
export fn unsign(signed_cookie: string, secret: string): UnsignResult;
