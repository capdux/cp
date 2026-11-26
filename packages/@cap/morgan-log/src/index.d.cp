// Type definitions for CP

import { formatU64 } from "../../itoa/src/index.cp";
export fn mrgStrEq(a: string, b: string): bool;
export fn mrgIsTokenChar(c: u8): bool;
export fn mrgWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn mrgWriteSlice(out: []u8, pos: usize, s: string): usize;
export fn mrgWriteOrDash(out: []u8, pos: usize, s: string): usize;
export fn mrgWriteU64(out: []u8, pos: usize, val: u64): usize;
export fn mrgWriteResponseTime(out: []u8, pos: usize, us: u64): usize;
export class HttpLogEvent {
    remote_addr: string,
    remote_user: string,
    date: string,
    method: string,
    url: string,
    http_version: string,
    status: u16,
    content_length: u64,
    has_content_length: bool,
    referrer: string,
    user_agent: string,
    response_time_us: u64,
}

export fn formatMorganLog(format_str: string, ev: HttpLogEvent, out: []u8): usize;
export fn formatPreset(preset: string, ev: HttpLogEvent, out: []u8): usize;
