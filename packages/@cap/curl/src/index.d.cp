// Type definitions for CP

import {  } from "std";
export fn cp_curl_version(): [*:0]&u8!;
export fn cp_curl_global_init(): i32;
export fn cp_curl_global_cleanup();
export fn cp_curl_http_get(url: [*:0]&u8!, out_status: &mut i64, out_len: &mut usize): [*]u8!?;
export fn cp_curl_free(ptr: [*]u8!);
export error CurlError { InitFailed, UrlTooLong, RequestFailed }

export class CurlResponse {
    status: i64,
    ptr: [*]u8!?,
    len: usize,
    fn statusCode(): i64;
    fn body(): string;
    fn length(): usize;
    fn __cp_deinit(_: &mut CurlResponse);
}

export fn version(): string;
export fn globalInit(): !void;
export fn globalCleanup();
export fn httpGet(url: string): !CurlResponse;
