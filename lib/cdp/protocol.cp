// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/cdp/Connection.ts | Synced: 2026-11-28
// ============================================================================
// lib/cdp/protocol.cp — Pure CP Chrome DevTools Protocol (JSON-RPC 2.0) Core
// Zero-Heap Decoding + 16-Lane @Vector(16, u8) SIMD Frame Scanner & ID Parser
// ============================================================================

const std = @import("std");

type CdpVec16 = @Vector(16, u8);

pub err CdpError {
    InvalidFrame,
    InvalidJson,
    MissingId,
    MissingMethod,
    ProtocolError,
    SessionNotFound,
    SessionClosed,
    ConnectionClosed,
    NotConnected,
    Timeout,
    BufferTooSmall,
    InvalidUrl,
    ExecutableNotFound,
    OutOfMemory,
}

pub enum CdpMessageKind {
    Request,
    Response,
    ErrorResponse,
    Event,
}

pub struct CdpRequest {
    id: i64 = 0,
    method: []const u8 = "",
    params_json: []const u8 = "{}",
    session_id: []const u8 = "",
}

pub struct CdpResponse {
    id: i64 = 0,
    result_json: []const u8 = "{}",
    error_code: i64 = 0,
    error_message: []const u8 = "",
    session_id: []const u8 = "",
    is_error: bool = false,
}

pub struct CdpEvent {
    method: []const u8 = "",
    params_json: []const u8 = "{}",
    session_id: []const u8 = "",
}

pub struct CdpMessage {
    kind: CdpMessageKind = CdpMessageKind.Event,
    id: i64 = 0,
    method: []const u8 = "",
    params_json: []const u8 = "{}",
    result_json: []const u8 = "{}",
    error_code: i64 = 0,
    error_message: []const u8 = "",
    session_id: []const u8 = "",

    pub fn isResponse(): bool {
        return _.kind == CdpMessageKind.Response || _.kind == CdpMessageKind.ErrorResponse;
    }

    pub fn isEvent(): bool {
        return _.kind == CdpMessageKind.Event;
    }

    pub fn isError(): bool {
        return _.kind == CdpMessageKind.ErrorResponse;
    }

    pub fn toResponse(): CdpResponse {
        return CdpResponse{
            id: _.id,
            result_json: _.result_json,
            error_code: _.error_code,
            error_message: _.error_message,
            session_id: _.session_id,
            is_error: _.kind == CdpMessageKind.ErrorResponse,
        };
    }

    pub fn toEvent(): CdpEvent {
        return CdpEvent{
            method: _.method,
            params_json: _.params_json,
            session_id: _.session_id,
        };
    }
}

pub struct CdpFrameScanner {
    /// 16-lane SIMD scan for '"' or '\\' inside JSON strings.
    pub static fn indexOfQuoteOrEscape(input: []const u8): usize? {
        let i: usize = 0;
        const n = input.len;
        const t1: CdpVec16 = @splat('"');
        const t2: CdpVec16 = @splat('\\');

        while (i + 16 <= n) {
            const chunk: CdpVec16 = input[i..][0..16].*;
            const m1: u16 = @bitCast(chunk == t1);
            const m2: u16 = @bitCast(chunk == t2);
            const mask: u16 = m1 | m2;
            if (mask != 0) {
                return i + (@ctz(mask) as usize);
            }
            i += 16;
        }

        while (i < n) {
            const c = input[i];
            if (c == '"' || c == '\\') {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// 16-lane SIMD scan for JSON structural delimiters: '{', '}', '[', ']', '"', ',', ':'.
    pub static fn indexOfStructural(input: []const u8): usize? {
        let i: usize = 0;
        const n = input.len;
        const v_lbrace: CdpVec16 = @splat('{');
        const v_rbrace: CdpVec16 = @splat('}');
        const v_lbrack: CdpVec16 = @splat('[');
        const v_rbrack: CdpVec16 = @splat(']');
        const v_quote: CdpVec16 = @splat('"');
        const v_comma: CdpVec16 = @splat(',');
        const v_colon: CdpVec16 = @splat(':');

        while (i + 16 <= n) {
            const chunk: CdpVec16 = input[i..][0..16].*;
            const m1: u16 = @bitCast(chunk == v_lbrace);
            const m2: u16 = @bitCast(chunk == v_rbrace);
            const m3: u16 = @bitCast(chunk == v_lbrack);
            const m4: u16 = @bitCast(chunk == v_rbrack);
            const m5: u16 = @bitCast(chunk == v_quote);
            const m6: u16 = @bitCast(chunk == v_comma);
            const m7: u16 = @bitCast(chunk == v_colon);
            const mask: u16 = m1 | m2 | m3 | m4 | m5 | m6 | m7;
            if (mask != 0) {
                return i + (@ctz(mask) as usize);
            }
            i += 16;
        }

        while (i < n) {
            const c = input[i];
            if (c == '{' || c == '}' || c == '[' || c == ']' || c == '"' || c == ',' || c == ':') {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// Skip whitespace (space, tab, CR, LF).
    pub static fn skipWs(input: []const u8, start: usize): usize {
        let i = start;
        while (i < input.len) {
            const c = input[i];
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                return i;
            }
            i += 1;
        }
        return i;
    }

    /// Trim leading and trailing JSON whitespace.
    pub static fn trimWs(input: []const u8): []const u8 {
        let s = CdpFrameScanner.skipWs(input, 0);
        let e = input.len;
        while (e > s) {
            const c = input[e - 1];
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                break;
            }
            e -= 1;
        }
        return input[s..e];
    }

    /// Scan past a JSON string starting at the opening '"' index, returning the index of the closing '"'.
    pub static fn scanStringEnd(input: []const u8, open_quote: usize): usize? {
        if (open_quote >= input.len || input[open_quote] != '"') {
            return null;
        }
        let i = open_quote + 1;
        while (i < input.len) {
            if (rel := CdpFrameScanner.indexOfQuoteOrEscape(input[i..])) {
                const pos = i + rel;
                if (input[pos] == '"') {
                    return pos;
                }
                // Backslash escape: skip escaped char
                i = pos + 2;
            } else {
                return null;
            }
        }
        return null;
    }

    /// Find matching '}' or ']' starting at `open_idx`, skipping nested strings via SIMD.
    pub static fn findMatchingContainer(input: []const u8, open_idx: usize): usize? {
        if (open_idx >= input.len) return null;
        const open_ch = input[open_idx];
        if (open_ch != '{' && open_ch != '[') return null;

        let depth: usize = 0;
        let i = open_idx;
        while (i < input.len) {
            if (rel := CdpFrameScanner.indexOfStructural(input[i..])) {
                const pos = i + rel;
                const c = input[pos];
                if (c == '"') {
                    if (end_q := CdpFrameScanner.scanStringEnd(input, pos)) {
                        i = end_q + 1;
                    } else {
                        return null;
                    }
                } else if (c == '{' || c == '[') {
                    depth += 1;
                    i = pos + 1;
                } else if (c == '}' || c == ']') {
                    if (depth == 0) return null;
                    depth -= 1;
                    if (depth == 0) {
                        return pos;
                    }
                    i = pos + 1;
                } else {
                    i = pos + 1;
                }
            } else {
                return null;
            }
        }
        return null;
    }

    /// Parse a signed 64-bit integer from ASCII slice (fast path for JSON-RPC id / error code).
    pub static fn parseFastI64(raw: []const u8): i64? {
        const s = CdpFrameScanner.trimWs(raw);
        if (s.len == 0) return null;
        let neg = false;
        let i: usize = 0;
        if (s[0] == '-') {
            neg = true;
            i = 1;
        } else if (s[0] == '+') {
            i = 1;
        }
        if (i >= s.len) return null;
        let acc: i64 = 0;
        while (i < s.len) {
            const c = s[i];
            if (c < '0' || c > '9') return null;
            const digit: i64 = (c - '0') as i64;
            acc = acc * 10 + digit;
            i += 1;
        }
        return if (neg) -acc else acc;
    }

    /// Zero-heap top-level JSON object field extractor.
    /// Returns raw value slice (strings unquoted if `unquote_strings == true`, objects/arrays/numbers/bools verbatim).
    pub static fn extractTopLevelField(json_obj: []const u8, target_key: []const u8, unquote_strings: bool): ([]const u8)? {
        const trimmed = CdpFrameScanner.trimWs(json_obj);
        if (trimmed.len < 2 || trimmed[0] != '{') return null;

        let i = CdpFrameScanner.skipWs(trimmed, 1);
        while (i < trimmed.len) {
            if (trimmed[i] == '}') return null;
            if (trimmed[i] == ',') {
                i = CdpFrameScanner.skipWs(trimmed, i + 1);
                continue;
            }
            if (trimmed[i] != '"') return null;

            const key_end = CdpFrameScanner.scanStringEnd(trimmed, i) orelse return null;
            const key_slice = trimmed[i + 1 .. key_end];
            i = CdpFrameScanner.skipWs(trimmed, key_end + 1);
            if (i >= trimmed.len || trimmed[i] != ':') return null;
            i = CdpFrameScanner.skipWs(trimmed, i + 1);
            if (i >= trimmed.len) return null;

            const val_start = i;
            let val_end = val_start;
            let is_string = false;

            const ch = trimmed[val_start];
            if (ch == '"') {
                is_string = true;
                const str_end = CdpFrameScanner.scanStringEnd(trimmed, val_start) orelse return null;
                val_end = str_end + 1;
            } else if (ch == '{' || ch == '[') {
                const close_idx = CdpFrameScanner.findMatchingContainer(trimmed, val_start) orelse return null;
                val_end = close_idx + 1;
            } else {
                // Primitive: number, true, false, null
                while (val_end < trimmed.len) {
                    const pc = trimmed[val_end];
                    if (pc == ',' || pc == '}' || pc == ' ' || pc == '\t' || pc == '\r' || pc == '\n') {
                        break;
                    }
                    val_end += 1;
                }
            }

            if (std.mem.eql(u8, key_slice, target_key)) {
                if (is_string && unquote_strings && val_end >= val_start + 2) {
                    return trimmed[val_start + 1 .. val_end - 1];
                }
                return trimmed[val_start..val_end];
            }

            i = CdpFrameScanner.skipWs(trimmed, val_end);
        }
        return null;
    }

    /// Decode a raw JSON-RPC 2.0 / CDP frame into CdpMessage without any heap allocation.
    pub static fn decodeMessage(raw: []const u8): CdpMessage!CdpError {
        const trimmed = CdpFrameScanner.trimWs(raw);
        if (trimmed.len < 2 || trimmed[0] != '{') {
            return CdpError.InvalidFrame;
        }

        const id_slice = CdpFrameScanner.extractTopLevelField(trimmed, "id", false);
        const method_slice = CdpFrameScanner.extractTopLevelField(trimmed, "method", true);
        const session_slice = CdpFrameScanner.extractTopLevelField(trimmed, "sessionId", true);
        const session_id = session_slice orelse "";

        if (id_raw := id_slice) {
            const parsed_id = CdpFrameScanner.parseFastI64(id_raw) orelse return CdpError.InvalidJson;

            // If "method" is present alongside "id", it is a Request.
            if (m := method_slice) {
                const params_raw = CdpFrameScanner.extractTopLevelField(trimmed, "params", false) orelse "{}";
                return CdpMessage{
                    kind: CdpMessageKind.Request,
                    id: parsed_id,
                    method: m,
                    params_json: params_raw,
                    session_id: session_id,
                };
            }

            // Check for "error" object
            if (err_obj := CdpFrameScanner.extractTopLevelField(trimmed, "error", false)) {
                let err_code: i64 = -1;
                if (code_raw := CdpFrameScanner.extractTopLevelField(err_obj, "code", false)) {
                    if (c := CdpFrameScanner.parseFastI64(code_raw)) {
                        err_code = c;
                    }
                }
                const err_msg = CdpFrameScanner.extractTopLevelField(err_obj, "message", true) orelse "Protocol error";
                return CdpMessage{
                    kind: CdpMessageKind.ErrorResponse,
                    id: parsed_id,
                    error_code: err_code,
                    error_message: err_msg,
                    session_id: session_id,
                };
            }

            // Standard Response
            const result_raw = CdpFrameScanner.extractTopLevelField(trimmed, "result", false) orelse "{}";
            return CdpMessage{
                kind: CdpMessageKind.Response,
                id: parsed_id,
                result_json: result_raw,
                session_id: session_id,
            };
        }

        // No "id" -> Event
        const ev_method = method_slice orelse return CdpError.MissingMethod;
        const ev_params = CdpFrameScanner.extractTopLevelField(trimmed, "params", false) orelse "{}";
        return CdpMessage{
            kind: CdpMessageKind.Event,
            method: ev_method,
            params_json: ev_params,
            session_id: session_id,
        };
    }

    /// Encode a CdpRequest into a caller-provided stack/arena buffer (zero-heap).
    pub static fn encodeRequest(req: CdpRequest, buf: []u8): usize!CdpError {
        if (req.method.len == 0) return CdpError.MissingMethod;
        const params_slice = if (req.params_json.len > 0) req.params_json else "{}";

        let written: usize = 0;
        if (req.session_id.len > 0) {
            const s = std.fmt.bufPrint(
                buf,
                "{{\"id\":{d},\"method\":\"{s}\",\"params\":{s},\"sessionId\":\"{s}\"}}",
                .{ req.id, req.method, params_slice, req.session_id },
            ) catch return CdpError.BufferTooSmall;
            written = s.len;
        } else {
            const s = std.fmt.bufPrint(
                buf,
                "{{\"id\":{d},\"method\":\"{s}\",\"params\":{s}}}",
                .{ req.id, req.method, params_slice },
            ) catch return CdpError.BufferTooSmall;
            written = s.len;
        }
        return written;
    }

    /// Encode a CdpRequest into a heap-allocated byte slice.
    pub static fn encodeRequestAlloc(alloc: std.mem.Allocator, req: CdpRequest): ([]u8)!CdpError {
        if (req.method.len == 0) return CdpError.MissingMethod;
        const params_slice = if (req.params_json.len > 0) req.params_json else "{}";
        if (req.session_id.len > 0) {
            return std.fmt.allocPrint(
                alloc,
                "{{\"id\":{d},\"method\":\"{s}\",\"params\":{s},\"sessionId\":\"{s}\"}}",
                .{ req.id, req.method, params_slice, req.session_id },
            ) catch return CdpError.OutOfMemory;
        }
        return std.fmt.allocPrint(
            alloc,
            "{{\"id\":{d},\"method\":\"{s}\",\"params\":{s}}}",
            .{ req.id, req.method, params_slice },
        ) catch return CdpError.OutOfMemory;
    }
}
