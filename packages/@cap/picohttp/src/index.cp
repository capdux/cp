// @cap/picohttp — PicoHTTPParser C-FFI & RAII safe wrapper for CP
// License: MIT (Upstream: H2O / picohttpparser)

import std from "std";

pub @c struct phr_header {
    name: [*]const u8?! = null;
    name_len: usize = 0;
    value: [*]const u8?! = null;
    value_len: usize = 0;
}

pub @c struct phr_chunked_decoder {
    bytes_left_in_chunk: usize = 0;
    consume_trailer: u8 = 0;
    _hex_count: u8 = 0;
    _state: u8 = 0;
}

pub @c {
    fn phr_parse_request(
        buf: [*]const u8!,
        len: usize,
        method: *([*]const u8?!)!,
        method_len: *usize!,
        path: *([*]const u8?!)!,
        path_len: *usize!,
        minor_version: *i32!,
        headers: [*]phr_header!,
        num_headers: *usize!,
        last_len: usize
    ): i32;

    fn phr_parse_response(
        buf: [*]const u8!,
        len: usize,
        minor_version: *i32!,
        status: *i32!,
        msg: *([*]const u8?!)!,
        msg_len: *usize!,
        headers: [*]phr_header!,
        num_headers: *usize!,
        last_len: usize
    ): i32;

    fn phr_decode_chunked(
        decoder: *phr_chunked_decoder!,
        buf: [*]u8!,
        bufsz: *usize!
    ): isize;

    fn phr_decode_chunked_is_in_data(decoder: *phr_chunked_decoder!): i32;
}

pub err PicoHttpError {
    OutOfMemory,
    NotInitialized,
    ParseFailed,
    Incomplete,
    InvalidChunkedEncoding,
}

pub struct ParsedHeader {
    pub name: []const u8 = "";
    pub value: []const u8 = "";
}

pub struct ParsedRequest {
    pub method: []const u8 = "";
    pub path: []const u8 = "";
    pub minor_version: i32 = 1;
    pub header_count: usize = 0;
    pub bytes_read: usize = 0;
}

pub struct ParsedResponse {
    pub minor_version: i32 = 1;
    pub status: i32 = 0;
    pub message: []const u8 = "";
    pub header_count: usize = 0;
    pub bytes_read: usize = 0;
}

/// Reusable HTTP/1.x zero-copy parser owning a heap header scratch buffer with deterministic `~init()` RAII cleanup.
pub class HttpParser {
    pub headers_ptr: [*]phr_header?! = null,
    pub max_headers: usize = 0,
    pub allocator: std.mem.Allocator = std.heap.page_allocator,

    pub fn init(*_, max_headers: usize): void! {
        const cap = if (max_headers == 0) 64 else max_headers;
        const alloc = std.heap.page_allocator;
        const buf = alloc.alloc(phr_header, cap) catch return PicoHttpError.OutOfMemory;
        _.allocator = alloc;
        _.headers_ptr = buf.ptr;
        _.max_headers = cap;
    }

    pub fn parseRequest(*_, raw: []const u8): ParsedRequest! {
        const hptr = _.headers_ptr orelse return PicoHttpError.NotInitialized;
        let method_ptr: [*]const u8?! = null;
        let method_len: usize = 0;
        let path_ptr: [*]const u8?! = null;
        let path_len: usize = 0;
        let minor_version: i32 = 0;
        let num_headers: usize = _.max_headers;

        const rc = phr_parse_request(
            raw.ptr,
            raw.len,
            &method_ptr,
            &method_len,
            &path_ptr,
            &path_len,
            &minor_version,
            hptr,
            &num_headers,
            0
        );
        if (rc == -2) return PicoHttpError.Incomplete;
        if (rc < 0) return PicoHttpError.ParseFailed;

        let m_slice: []const u8 = "";
        if (mp := method_ptr) {
            m_slice = mp[0..method_len];
        }
        let p_slice: []const u8 = "";
        if (pp := path_ptr) {
            p_slice = pp[0..path_len];
        }
        return ParsedRequest {
            method: m_slice,
            path: p_slice,
            minor_version: minor_version,
            header_count: num_headers,
            bytes_read: rc as usize,
        };
    }

    pub fn parseResponse(*_, raw: []const u8): ParsedResponse! {
        const hptr = _.headers_ptr orelse return PicoHttpError.NotInitialized;
        let minor_version: i32 = 0;
        let status: i32 = 0;
        let msg_ptr: [*]const u8?! = null;
        let msg_len: usize = 0;
        let num_headers: usize = _.max_headers;

        const rc = phr_parse_response(
            raw.ptr,
            raw.len,
            &minor_version,
            &status,
            &msg_ptr,
            &msg_len,
            hptr,
            &num_headers,
            0
        );
        if (rc == -2) return PicoHttpError.Incomplete;
        if (rc < 0) return PicoHttpError.ParseFailed;

        let msg_slice: []const u8 = "";
        if (mp := msg_ptr) {
            msg_slice = mp[0..msg_len];
        }
        return ParsedResponse {
            minor_version: minor_version,
            status: status,
            message: msg_slice,
            header_count: num_headers,
            bytes_read: rc as usize,
        };
    }

    pub fn headerAt(index: usize): ParsedHeader? {
        const hptr = _.headers_ptr orelse return null;
        if (index >= _.max_headers) return null;
        const h = unsafe { hptr[index] };
        let n: []const u8 = "";
        if (np := h.name) {
            n = unsafe { np[0..h.name_len] };
        }
        let v: []const u8 = "";
        if (vp := h.value) {
            v = unsafe { vp[0..h.value_len] };
        }
        return ParsedHeader { name: n, value: v };
    }

    ~init() {
        if (hptr := _.headers_ptr) {
            unsafe { _.allocator.free(hptr[0.._.max_headers]); }
            _.headers_ptr = null;
            _.max_headers = 0;
        }
    }
}

/// Stateful HTTP/1.1 chunked transfer-encoding decoder with deterministic `~init()` state reset.
pub class ChunkedDecoder {
    pub inner: phr_chunked_decoder = undefined,

    pub fn init(*_, consume_trailer: bool) {
        _.inner.bytes_left_in_chunk = 0;
        _.inner.consume_trailer = if (consume_trailer) 1 else 0;
        _.inner._hex_count = 0;
        _.inner._state = 0;
    }

    pub fn decode(*_, buf: []u8): usize! {
        let sz: usize = buf.len;
        const rc = phr_decode_chunked(&_.inner, buf.ptr, &sz);
        if (rc == -1) return PicoHttpError.InvalidChunkedEncoding;
        if (rc == -2) return PicoHttpError.Incomplete;
        return sz;
    }

    pub fn isInData(*_): bool {
        return phr_decode_chunked_is_in_data(&_.inner) != 0;
    }

    ~init() {
        _.inner.bytes_left_in_chunk = 0;
        _.inner.consume_trailer = 0;
        _.inner._hex_count = 0;
        _.inner._state = 0;
    }
}
