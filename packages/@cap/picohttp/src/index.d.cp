// Type definitions for CP

import {  } from "std";
export class phr_header {
    name: [*]&u8!?,
    name_len: usize,
    value: [*]&u8!?,
    value_len: usize,
}

export class phr_chunked_decoder {
    bytes_left_in_chunk: usize,
    consume_trailer: u8,
    _hex_count: u8,
    _state: u8,
}

export fn phr_parse_request(buf: [*]&u8!, len: usize, method: *[*]&u8!?!, method_len: *usize!, path: *[*]&u8!?!, path_len: *usize!, minor_version: *i32!, headers: [*]phr_header!, num_headers: *usize!, last_len: usize): i32;
export fn phr_parse_response(buf: [*]&u8!, len: usize, minor_version: *i32!, status: *i32!, msg: *[*]&u8!?!, msg_len: *usize!, headers: [*]phr_header!, num_headers: *usize!, last_len: usize): i32;
export fn phr_decode_chunked(decoder: *phr_chunked_decoder!, buf: [*]u8!, bufsz: *usize!): isize;
export fn phr_decode_chunked_is_in_data(decoder: *phr_chunked_decoder!): i32;
export error PicoHttpError { OutOfMemory, NotInitialized, ParseFailed, Incomplete, InvalidChunkedEncoding }

export class ParsedHeader {
    name: &[]u8,
    value: &[]u8,
}

export class ParsedRequest {
    method: &[]u8,
    path: &[]u8,
    minor_version: i32,
    header_count: usize,
    bytes_read: usize,
}

export class ParsedResponse {
    minor_version: i32,
    status: i32,
    message: &[]u8,
    header_count: usize,
    bytes_read: usize,
}

export class HttpParser {
    headers_ptr: [*]phr_header!?,
    max_headers: usize,
    allocator: std.mem.Allocator,
    fn init(_: auto, max_headers: usize): !void;
    fn parseRequest(_: auto, raw: &[]u8): !ParsedRequest;
    fn parseResponse(_: auto, raw: &[]u8): !ParsedResponse;
    fn headerAt(index: usize): ParsedHeader?;
    fn __cp_deinit(_: &mut HttpParser);
}

export class ChunkedDecoder {
    inner: phr_chunked_decoder,
    fn init(_: auto, consume_trailer: bool);
    fn decode(_: auto, buf: []u8): !usize;
    fn isInData(_: auto): bool;
    fn __cp_deinit(_: &mut ChunkedDecoder);
}

