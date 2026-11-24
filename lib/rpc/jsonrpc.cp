// Reference-hardened against paritytech/jsonrpsee (MIT) & JSON-RPC 2.0 / MCP
// Source: cp/lib/rpc/jsonrpc.cp (JSON-RPC 2.0 Wire Codec & Standard Error Codes)
// Pure CP RFC-compliant JSON-RPC 2.0 codec with typed IDs, standard error codes,
// request/notification/response parsing & zero-leak Arena-isolated DOM validation.

const std = @import("std");
import { JsonValue, JsonTag } from "../json/value.cp";
import { JsonParser } from "../json/parser.cp";
import { JsonSerializer } from "../json/serializer.cp";

pub const ERR_PARSE: i32 = -32700;
pub const ERR_INVALID_REQUEST: i32 = -32600;
pub const ERR_METHOD_NOT_FOUND: i32 = -32601;
pub const ERR_INVALID_PARAMS: i32 = -32602;
pub const ERR_INTERNAL: i32 = -32603;

pub err JsonRpcCodecError {
    ParseError,
    InvalidRequest,
    InvalidVersion,
    InvalidId,
    InvalidMethod,
    InvalidParams,
    InvalidResponse,
    OutOfMemory,
}

pub enum JsonRpcId {
    Null,
    Int: i64,
    Str: string,

    fn isNull(): bool {
        return switch (_) {
            .Null => true,
            else => false,
        };
    }

    fn asInt(): i64? {
        return switch (_) {
            .Int v => v,
            else => null,
        };
    }

    fn asStr(): string? {
        return switch (_) {
            .Str s => s,
            else => null,
        };
    }

    fn equals(other: JsonRpcId): bool {
        return switch (_) {
            .Null => switch (other) {
                .Null => true,
                else => false,
            },
            .Int v => switch (other) {
                .Int ov => v == ov,
                else => false,
            },
            .Str s => switch (other) {
                .Str os => std.mem.eql(u8, s, os),
                else => false,
            },
        };
    }

    fn clone(alloc: std.mem.Allocator): JsonRpcId! {
        return switch (_) {
            .Null => JsonRpcId.Null,
            .Int v => JsonRpcId.Int(v),
            .Str s => {
                const duped = alloc.dupe(u8, s)?;
                return JsonRpcId.Str(duped);
            },
        };
    }

    fn deinit(*_, alloc: std.mem.Allocator) {
        switch (_.*) {
            .Str s => {
                if (s.len > 0) alloc.free(s);
            },
            else => {},
        }
        _.* = JsonRpcId.Null;
    }
}

pub struct JsonRpcError {
    code: i32 = ERR_INTERNAL,
    message: string = "",
    data_json: string? = null,

    pub static fn init(code: i32, message: string, data_json: string? = null): JsonRpcError {
        return JsonRpcError{
            code: code,
            message: message,
            data_json: data_json,
        };
    }

    pub static fn parseError(msg: string = "Parse error"): JsonRpcError {
        return JsonRpcError{ code: ERR_PARSE, message: msg, data_json: null };
    }

    pub static fn invalidRequest(msg: string = "Invalid Request"): JsonRpcError {
        return JsonRpcError{ code: ERR_INVALID_REQUEST, message: msg, data_json: null };
    }

    pub static fn methodNotFound(msg: string = "Method not found"): JsonRpcError {
        return JsonRpcError{ code: ERR_METHOD_NOT_FOUND, message: msg, data_json: null };
    }

    pub static fn invalidParams(msg: string = "Invalid params"): JsonRpcError {
        return JsonRpcError{ code: ERR_INVALID_PARAMS, message: msg, data_json: null };
    }

    pub static fn internalError(msg: string = "Internal error"): JsonRpcError {
        return JsonRpcError{ code: ERR_INTERNAL, message: msg, data_json: null };
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.message.len > 0) alloc.free(_.message);
        if (d := _.data_json) {
            if (d.len > 0) alloc.free(d);
        }
        _.message = "";
        _.data_json = null;
    }
}

pub struct JsonRpcRequest {
    id: JsonRpcId = JsonRpcId.Null,
    method: string = "",
    params_json: string = "",
    has_id: bool = false,

    pub fn isNotification(): bool {
        if (!_.has_id) {
            return switch (_.id) {
                .Null => true,
                else => false,
            };
        }
        return false;
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.id.deinit(alloc);
        if (_.method.len > 0) alloc.free(_.method);
        if (_.params_json.len > 0) alloc.free(_.params_json);
        _.method = "";
        _.params_json = "";
        _.has_id = false;
    }
}

pub struct JsonRpcResponse {
    id: JsonRpcId = JsonRpcId.Null,
    result_json: string? = null,
    rpc_error: JsonRpcError? = null,

    pub fn isSuccess(): bool {
        return _.rpc_error == null && _.result_json != null;
    }

    pub fn isError(): bool {
        return _.rpc_error != null;
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.id.deinit(alloc);
        if (res := _.result_json) {
            if (res.len > 0) alloc.free(res);
        }
        if (*err_val := _.rpc_error) {
            err_val.deinit(alloc);
        }
        _.result_json = null;
        _.rpc_error = null;
    }
}

pub struct JsonRpcCodec {
    pub static fn appendEscapedJsonString(out: *std.ArrayList(u8), alloc: std.mem.Allocator, s: string): ! {
        out.append(alloc, '"')?;
        let i: usize = 0;
        let flush_start: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            if (c == '"' || c == '\\' || c < 0x20) {
                if (i > flush_start) {
                    out.appendSlice(alloc, s[flush_start..i])?;
                }
                switch (c) {
                    '"' => out.appendSlice(alloc, "\\\"")?,
                    '\\' => out.appendSlice(alloc, "\\\\")?,
                    0x08 => out.appendSlice(alloc, "\\b")?,
                    0x0C => out.appendSlice(alloc, "\\f")?,
                    '\n' => out.appendSlice(alloc, "\\n")?,
                    '\r' => out.appendSlice(alloc, "\\r")?,
                    '\t' => out.appendSlice(alloc, "\\t")?,
                    else => {
                        const hex = "0123456789abcdef";
                        let seq: [6]u8 = ['\\', 'u', '0', '0', hex[(c >> 4) & 0xF], hex[c & 0xF]];
                        out.appendSlice(alloc, seq[0..6])?;
                    },
                }
                flush_start = i + 1;
            }
        }
        if (i > flush_start) {
            out.appendSlice(alloc, s[flush_start..i])?;
        }
        out.append(alloc, '"')?;
    }

    static fn appendI64(out: *std.ArrayList(u8), alloc: std.mem.Allocator, v: i64): ! {
        let buf: [32]u8 = [0; 32];
        let idx: usize = 32;
        const neg = v < 0;
        let mag: u64 = if (neg) @as(u64, @intCast(-@as(i128, v))) else @as(u64, @intCast(v));
        if (mag == 0) {
            idx -= 1;
            buf[idx] = '0';
        } else {
            while (mag > 0) {
                idx -= 1;
                buf[idx] = @as(u8, '0') + @as(u8, @intCast(mag % 10));
                mag /= 10;
            }
        }
        if (neg) {
            idx -= 1;
            buf[idx] = '-';
        }
        out.appendSlice(alloc, buf[idx..32])?;
    }

    static fn appendId(out: *std.ArrayList(u8), alloc: std.mem.Allocator, id: JsonRpcId): ! {
        switch (id) {
            .Null => out.appendSlice(alloc, "null")?,
            .Int v => JsonRpcCodec.appendI64(out, alloc, v)?,
            .Str s => JsonRpcCodec.appendEscapedJsonString(out, alloc, s)?,
        }
    }

    static fn parseExactInt(num: f64): i64? {
        if (!std.math.isFinite(num) || @trunc(num) != num) {
            return null;
        }
        if (num < -9007199254740991.0 || num > 9007199254740991.0) {
            return null;
        }
        return @as(i64, @intFromFloat(num));
    }

    static fn parseIdValue(alloc: std.mem.Allocator, val: JsonValue): JsonRpcId! {
        return switch (val.tag) {
            .Null => JsonRpcId.Null,
            .Number => {
                const int_v = JsonRpcCodec.parseExactInt(val.num_val) ?? return JsonRpcCodecError.InvalidId;
                return JsonRpcId.Int(int_v);
            },
            .String => {
                const duped = alloc.dupe(u8, val.str_val)?;
                return JsonRpcId.Str(duped);
            },
            else => return JsonRpcCodecError.InvalidId,
        };
    }

    pub static fn parseRequest(alloc: std.mem.Allocator, json_str: string): JsonRpcRequest! {
        let tmp_arena = std.heap.ArenaAllocator.init(alloc);
        defer tmp_arena.deinit();
        const arena_alloc = tmp_arena.allocator();

        const root = JsonParser.parse(arena_alloc, json_str) catch return JsonRpcCodecError.ParseError;
        if (!root.isObject()) {
            return JsonRpcCodecError.InvalidRequest;
        }

        const ver_val = root.get("jsonrpc") ?? return JsonRpcCodecError.InvalidVersion;
        const ver_str = ver_val.asString() ?? return JsonRpcCodecError.InvalidVersion;
        if (!std.mem.eql(u8, ver_str, "2.0")) {
            return JsonRpcCodecError.InvalidVersion;
        }

        const method_val = root.get("method") ?? return JsonRpcCodecError.InvalidMethod;
        const method_str = method_val.asString() ?? return JsonRpcCodecError.InvalidMethod;
        if (method_str.len == 0) {
            return JsonRpcCodecError.InvalidMethod;
        }

        let has_id = false;
        let parsed_id = JsonRpcId.Null;
        if (id_val := root.get("id")) {
            has_id = true;
            parsed_id = JsonRpcCodec.parseIdValue(alloc, id_val)?;
        }
        errdefer parsed_id.deinit(alloc);

        const owned_method = alloc.dupe(u8, method_str)?;
        errdefer alloc.free(owned_method);

        let owned_params: []u8 = [];
        if (params_val := root.get("params")) {
            if (!params_val.isObject() && !params_val.isArray()) {
                return JsonRpcCodecError.InvalidParams;
            }
            owned_params = JsonSerializer.stringify(alloc, params_val)?;
        }

        return JsonRpcRequest{
            id: parsed_id,
            method: owned_method,
            params_json: owned_params,
            has_id: has_id,
        };
    }

    pub static fn encodeRequest(alloc: std.mem.Allocator, req: JsonRpcRequest): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(alloc, "{\"jsonrpc\":\"2.0\"")?;
        if (!req.isNotification()) {
            out.appendSlice(alloc, ",\"id\":")?;
            JsonRpcCodec.appendId(&out, alloc, req.id)?;
        }
        out.appendSlice(alloc, ",\"method\":")?;
        JsonRpcCodec.appendEscapedJsonString(&out, alloc, req.method)?;

        const trimmed_params = std.mem.trim(u8, req.params_json, " \t\r\n");
        if (trimmed_params.len > 0) {
            out.appendSlice(alloc, ",\"params\":")?;
            out.appendSlice(alloc, trimmed_params)?;
        }
        out.append(alloc, '}')?;
        return out.toOwnedSlice(alloc);
    }

    pub static fn encodeSuccess(alloc: std.mem.Allocator, id: JsonRpcId, result_json: string): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(alloc, "{\"jsonrpc\":\"2.0\",\"id\":")?;
        JsonRpcCodec.appendId(&out, alloc, id)?;
        out.appendSlice(alloc, ",\"result\":")?;

        const trimmed = std.mem.trim(u8, result_json, " \t\r\n");
        if (trimmed.len > 0) {
            out.appendSlice(alloc, trimmed)?;
        } else {
            out.appendSlice(alloc, "null")?;
        }
        out.append(alloc, '}')?;
        return out.toOwnedSlice(alloc);
    }

    pub static fn encodeError(alloc: std.mem.Allocator, id: JsonRpcId, rpc_err: JsonRpcError): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(alloc, "{\"jsonrpc\":\"2.0\",\"id\":")?;
        JsonRpcCodec.appendId(&out, alloc, id)?;
        out.appendSlice(alloc, ",\"error\":{\"code\":")?;
        JsonRpcCodec.appendI64(&out, alloc, rpc_err.code as i64)?;
        out.appendSlice(alloc, ",\"message\":")?;
        JsonRpcCodec.appendEscapedJsonString(&out, alloc, rpc_err.message)?;

        if (data := rpc_err.data_json) {
            const trimmed = std.mem.trim(u8, data, " \t\r\n");
            if (trimmed.len > 0) {
                out.appendSlice(alloc, ",\"data\":")?;
                out.appendSlice(alloc, trimmed)?;
            }
        }
        out.appendSlice(alloc, "}}")?;
        return out.toOwnedSlice(alloc);
    }

    pub static fn parseResponse(alloc: std.mem.Allocator, json_str: string): JsonRpcResponse! {
        let tmp_arena = std.heap.ArenaAllocator.init(alloc);
        defer tmp_arena.deinit();
        const arena_alloc = tmp_arena.allocator();

        const root = JsonParser.parse(arena_alloc, json_str) catch return JsonRpcCodecError.ParseError;
        if (!root.isObject()) {
            return JsonRpcCodecError.InvalidResponse;
        }

        const ver_val = root.get("jsonrpc") ?? return JsonRpcCodecError.InvalidVersion;
        const ver_str = ver_val.asString() ?? return JsonRpcCodecError.InvalidVersion;
        if (!std.mem.eql(u8, ver_str, "2.0")) {
            return JsonRpcCodecError.InvalidVersion;
        }

        const id_val = root.get("id") ?? return JsonRpcCodecError.InvalidId;
        let parsed_id = JsonRpcCodec.parseIdValue(alloc, id_val)?;
        errdefer parsed_id.deinit(alloc);

        const res_opt = root.get("result");
        const err_opt = root.get("error");

        if (res_opt != null && err_opt != null) {
            return JsonRpcCodecError.InvalidResponse;
        }
        if (res_opt == null && err_opt == null) {
            return JsonRpcCodecError.InvalidResponse;
        }

        if (res_val := res_opt) {
            const owned_res = JsonSerializer.stringify(alloc, res_val)?;
            return JsonRpcResponse{
                id: parsed_id,
                result_json: owned_res,
                rpc_error: null,
            };
        }

        const err_val = err_opt.?;
        if (!err_val.isObject()) {
            return JsonRpcCodecError.InvalidResponse;
        }
        const code_val = err_val.get("code") ?? return JsonRpcCodecError.InvalidResponse;
        const code_num = code_val.asNumber() ?? return JsonRpcCodecError.InvalidResponse;
        const code_i64 = JsonRpcCodec.parseExactInt(code_num) ?? return JsonRpcCodecError.InvalidResponse;
        if (code_i64 < -2147483648 || code_i64 > 2147483647) {
            return JsonRpcCodecError.InvalidResponse;
        }

        const msg_val = err_val.get("message") ?? return JsonRpcCodecError.InvalidResponse;
        const msg_str = msg_val.asString() ?? return JsonRpcCodecError.InvalidResponse;
        const owned_msg = alloc.dupe(u8, msg_str)?;
        errdefer alloc.free(owned_msg);

        let owned_data: string? = null;
        if (data_val := err_val.get("data")) {
            owned_data = JsonSerializer.stringify(alloc, data_val)?;
        }

        return JsonRpcResponse{
            id: parsed_id,
            result_json: null,
            rpc_error: JsonRpcError{
                code: code_i64 as i32,
                message: owned_msg,
                data_json: owned_data,
            },
        };
    }
}
