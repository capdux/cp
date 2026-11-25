// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class AuditConfig {
    status_path: string,
    actor_header: string,
    mask_sensitive: bool,
}

export class AuditRecord {
    seq: u64,
    method: string,
    path: string,
    status: u16,
    actor: string,
    client_ip: string,
}

export class AudRingSlot {
    seq: u64,
    status: u16,
    method_buf: u8[AUD_MAX_METHOD_LEN],
    method_len: usize,
    path_buf: u8[AUD_MAX_PATH_LEN],
    path_len: usize,
    actor_buf: u8[AUD_MAX_ACTOR_LEN],
    actor_len: usize,
    ip_buf: u8[AUD_MAX_IP_LEN],
    ip_len: usize,
    used: bool,
}

export fn audAsciiLower(c: u8): u8;
export fn audAsciiEqlIgnoreCase(a: string, b: string): bool;
export fn audTrimSpaces(s: string): string;
export fn audCopyInto(dst: []u8, src: string): usize;
export fn audIsWriteMethod(m: AppMethod): bool;
export fn audResolveActor(ctx: &mut Context): string;
export fn audResolveClientIp(ctx: &mut Context): string;
export fn audWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
export fn audWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
export fn audWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;
export fn audWriteJsonStr(out_buf: []u8, pos: &mut usize, s: string): bool;
export fn auditMaskHeaderValue(header_name: string, val: string): string;
export fn auditCount(): usize;
export fn auditTotalSeq(): u64;
export fn auditReset();
export fn auditRecordAt(chronological_idx: usize): ?AuditRecord;
export fn audAfterHandle(ctx: &mut Context, resp: &mut Response);
export fn audHandleStatus(ctx: &mut Context): Response;
export fn audCreatePlugin(cfg: AuditConfig): Plugin;
export fn registerAudit(app: &mut App, cfg: AuditConfig): &mut App;
