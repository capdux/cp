// Type definitions for CP

import { escapeHtml } from "../../escape-html/src/index.cp";
import { StatusCode } from "../../http-types/src/index.cp";
import { formatU64 } from "../../itoa/src/index.cp";
export class FinalResponse {
    status: u16,
    body_len: usize,
    total_len: usize,
}

export fn fhPush(out: []u8, pos: usize, s: string): usize;
export fn fhPushU64(out: []u8, pos: usize, v: u64): usize;
export fn normalizeErrorStatus(code: u16): u16;
export fn statusReason(code: u16): string;
export fn formatErrorHtml(message: string, out: []u8): usize;
export fn formatCannotMessage(method: string, url: string, out: []u8): usize;
export fn buildFinalHttpResponse(status_code: u16, message: string, is_head: bool, out: []u8): FinalResponse;
