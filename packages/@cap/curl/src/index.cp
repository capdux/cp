// @cap/curl — libcurl (v8.23.0) Multiprotocol Network Transfer Library for CP
// License: curl (MIT-like, Upstream: curl/curl)

import std from "std";

pub @c {
    fn cp_curl_version(): [*:0]const u8;
    fn cp_curl_global_init(): i32;
    fn cp_curl_global_cleanup();
    fn cp_curl_http_get(url: [*:0]const u8, out_status: *i64, out_len: *usize): ([*]u8)?;
    fn cp_curl_free(ptr: [*]u8);
}

pub err CurlError {
    InitFailed,
    UrlTooLong,
    RequestFailed,
}

/// Owned RAII HTTP response returned by `httpGet`.
/// Automatically releases the underlying C buffer on scope exit via `~init()`.
pub struct CurlResponse {
    pub status: i64 = 0,
    ptr: ([*]u8)? = null,
    len: usize = 0,

    /// Returns the HTTP response status code (e.g. 200).
    pub fn statusCode(): i64 {
        return _.status;
    }

    /// Returns the response body as a byte slice (`string`).
    pub fn body(): string {
        if (p := _.ptr) {
            return p[0.._.len];
        }
        return "";
    }

    /// Returns the response body length in bytes.
    pub fn length(): usize {
        return _.len;
    }

    ~init() {
        if (p := _.ptr) {
            _.ptr = null;
            _.len = 0;
            unsafe {
                cp_curl_free(p);
            }
        }
    }
}

/// Returns the linked libcurl version string.
pub fn version(): string {
    unsafe {
        return std.mem.span(cp_curl_version());
    }
}

/// Initializes libcurl global state.
pub fn globalInit(): void! {
    let rc: i32 = 0;
    unsafe {
        rc = cp_curl_global_init();
    }
    if (rc != 0) {
        return CurlError.InitFailed;
    }
}

/// Cleans up libcurl global state.
pub fn globalCleanup() {
    unsafe {
        cp_curl_global_cleanup();
    }
}

/// Performs an HTTP GET request to `url` with built-in connect/transfer timeouts.
pub fn httpGet(url: string): CurlResponse! {
    if (url.len >= 4096) {
        return CurlError.UrlTooLong;
    }
    let url_buf: [4096]u8 = undefined;
    for (i of 0..url.len) {
        url_buf[i] = url[i];
    }
    url_buf[url.len] = 0;

    let status: i64 = 0;
    let body_len: usize = 0;
    let raw_ptr: ([*]u8)? = null;
    unsafe {
        const c_url: [*:0]const u8 = @ptrCast(&url_buf[0]);
        raw_ptr = cp_curl_http_get(c_url, &status, &body_len);
    }
    if (raw_ptr == null) {
        return CurlError.RequestFailed;
    }
    return CurlResponse{
        .status = status,
        .ptr = raw_ptr,
        .len = body_len,
    };
}
