// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class SessionConfig {
    secret: string,
    cookie_name: string,
    ttl_secs: u64,
    secure: bool,
    http_only: bool,
    same_site: string,
}

export class SessionSlot {
    used: bool,
    id_buf: u8[SESS_MAX_ID_LEN],
    id_len: usize,
    val_buf: u8[SESS_MAX_VAL_LEN],
    val_len: usize,
    created_at: u64,
    expires_at: u64,
    fn idSlice(): string;
    fn valSlice(): string;
    fn matchesId(id: string): bool;
}

export fn sessMonotonicNowSec(): u64;
export fn sessNowSec(): u64;
export fn sessFnv1a64Update(seed: u64, data: &[]u8): u64;
export fn sessSplitMix64(x: u64): u64;
export fn sessWriteHexU64(val: u64, out_buf: []u8, offset: usize);
export fn sessComputeMacHex(secret: string, id: string, out_hex: &mut u8[SESS_SIG_HEX_LEN]);
export fn sessConstTimeEql(a: string, b: string): bool;
export fn sessWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
export fn sessWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;
export fn sessVerifySignedCookie(signed_cookie: string): ?string;
export fn sessFindValidSlotIndex(id: string, now_sec: u64): ?usize;
export fn setSessionTestClock(now_sec: u64);
export fn resetSessionStore();
export fn sessionCreate(user_val: string, now_sec: u64, out_cookie_val: []u8): ?usize;
export fn sessionGet(signed_cookie: string, now_sec: u64): ?string;
export fn sessionDestroy(signed_cookie: string): bool;
export fn formatSessionSetCookie(cfg: SessionConfig, signed_cookie: string, out_buf: []u8): ?usize;
export fn sessionHook(ctx: &mut Context): bool;
export fn createSessionPlugin(cfg: SessionConfig): Plugin;
export fn registerSession(app: &mut App, cfg: SessionConfig): &mut App;
