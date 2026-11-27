// Type definitions for CP

import { escapeHtml } from "../../escape-html/src/index.cp";
import { StatusCode } from "../../http/src/index.cp";
import { formatU64 } from "../../itoa/src/index.cp";
export enum FinalEnv {
    Development,
    Production
}

export class ResponseHeader {
    name: string,
    value: string,
}

export class FinalError {
    status: u16,
    status_code: u16,
    expose: bool,
    message: string,
    stack: string,
}

export class FinalHandlerOptions {
    env: FinalEnv,
    is_head: bool,
    method: string,
    url: string,
    fn default(): FinalHandlerOptions;
}

export class FinalResponse {
    status: u16,
    body_len: usize,
    total_len: usize,
}

export fn fhPush(out: []u8, pos: usize, s: string): usize;
export fn fhPushU64(out: []u8, pos: usize, v: u64): usize;
export fn normalizeErrorStatus(code: u16): u16;
export fn statusCode(err: FinalError): u16;
export fn getErrorStatusCode(err: FinalError): u16;
export fn statusReason(code: u16): string;
export fn escapeHtmlText(input: string, out: []u8): usize;
export fn formatErrorMessage(err: FinalError, status: u16, env: FinalEnv): string;
export fn formatErrorHtml(message: string, out: []u8): usize;
export fn createHtmlDocument(message: string, out: []u8): usize;
export fn defaultSecurityHeaders(out: []ResponseHeader): usize;
export fn formatCannotMessage(method: string, url: string, out: []u8): usize;
export fn buildFinalHttpResponse(status_code: u16, message: string, is_head: bool, out: []u8): FinalResponse;
export fn buildFinalHttpResponseWithOptions(err: FinalError?, opts: FinalHandlerOptions, extra_headers: &[]ResponseHeader, out: []u8): FinalResponse;
