// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export fn rotr32(x: u32, n: u5): u32;
export fn sha256Compress(state: &mut [8]u32, block: &[64]u8);
export class Sha256Hasher {
    state: [8]u32,
    buf: [64]u8,
    buf_len: usize,
    total_len: u64,
    fn init(): Sha256Hasher;
    fn update(_: auto, data: &[]u8);
    fn finalize(_: auto, out: &mut [32]u8);
}

export fn sha256(data: &[]u8, out: &mut [32]u8);
export fn hmacSha256(key: string, msg: &[]u8, out: &mut [32]u8);
export fn base64UrlEncode(input: &[]u8, out_buf: []u8): usize?;
export fn base64UrlCharVal(ch: u8): u8?;
export fn base64UrlDecode(input: string, out_buf: []u8): usize?;
export fn jwtConstantTimeEql(a: &[]u8, b: &[]u8): bool;
export fn jwtAsciiLower(c: u8): u8;
export fn jwtAsciiEqlIgnoreCase(a: string, b: string): bool;
export fn jwtTrimSpaces(s: string): string;
export fn jwtContainsSubstr(haystack: string, needle: string): bool;
export fn jwtWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
export class JwtConfig {
    secret: string,
    issuer: string,
    header_name: string,
    optional_auth: bool,
}

export fn signHs256(secret: string, payload_json: string, out_buf: []u8): usize?;
export fn verifyHs256(secret: string, token: string, out_payload: []u8): usize?;
export fn jwtCheckIssuerClaim(payload: string, issuer: string): bool;
export fn configure(cfg: JwtConfig);
export fn authHook(ctx: &mut Context): bool;
export fn createJwtPlugin(cfg: JwtConfig): Plugin;
export fn registerJwt(app: &mut App, cfg: JwtConfig): &mut App;
