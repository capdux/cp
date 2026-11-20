// Source: cp/lib/mcp/protocol.cp (MCP 2025-03-26 JSON-RPC 2.0 protocol models and codec)
const std = @import("std");

pub const MCP_PROTOCOL_VERSION: []const u8 = "2025-03-26";

pub const JSONRPC_PARSE_ERROR: i32 = -32700;
pub const JSONRPC_INVALID_REQUEST: i32 = -32600;
pub const JSONRPC_METHOD_NOT_FOUND: i32 = -32601;
pub const JSONRPC_INVALID_PARAMS: i32 = -32602;
pub const JSONRPC_INTERNAL_ERROR: i32 = -32603;

pub struct McpError {
    code: i32,
    message: []const u8,
    data: ([]const u8)? = null,
}

pub struct McpCapabilities {
    tools: bool = false,
    resources: bool = false,
    prompts: bool = false,
    logging: bool = false,
}

pub struct McpToolDef {
    name: []const u8,
    description: []const u8,
    input_schema_json: []const u8 = "{\"type\":\"object\",\"properties\":{}}",
}

pub struct McpResourceDef {
    uri: []const u8,
    name: []const u8,
    description: []const u8 = "",
    mime_type: []const u8 = "text/plain",
}

pub struct McpPromptDef {
    name: []const u8,
    description: []const u8 = "",
    arguments_json: []const u8 = "[]",
}

pub struct McpContent {
    content_type: []const u8 = "text",
    text: []const u8,
    mime_type: ([]const u8)? = null,

    pub fn makeText(msg: []const u8): McpContent {
        return McpContent{
            .content_type = "text",
            .text = msg,
            .mime_type = null,
        };
    }
}

pub struct McpToolResult {
    content: []const McpContent,
    is_error: bool = false,

    pub fn ok(items: []const McpContent): McpToolResult {
        return McpToolResult{
            .content = items,
            .is_error = false,
        };
    }

    pub fn fail(items: []const McpContent): McpToolResult {
        return McpToolResult{
            .content = items,
            .is_error = true,
        };
    }
}

pub enum McpIdKind {
    None,
    Int,
    String,
}

pub struct McpId {
    kind: McpIdKind = McpIdKind.None,
    int_val: i64 = 0,
    str_val: []const u8 = "",

    pub fn none(): McpId {
        return McpId{ .kind = McpIdKind.None };
    }

    pub fn fromInt(v: i64): McpId {
        return McpId{ .kind = McpIdKind.Int, .int_val = v };
    }

    pub fn fromString(s: []const u8): McpId {
        return McpId{ .kind = McpIdKind.String, .str_val = s };
    }

    pub fn appendJson(list: *std.ArrayList(u8), alloc: std.mem.Allocator): !void {
        switch (_.kind) {
            McpIdKind.None => {
                try list.appendSlice(alloc, "null");
            },
            McpIdKind.Int => {
                const s = try std.fmt.allocPrint(alloc, "{d}", .{_.int_val});
                defer alloc.free(s);
                try list.appendSlice(alloc, s);
            },
            McpIdKind.String => {
                try appendJsonString(list, alloc, _.str_val);
            },
        }
    }
}

pub struct McpRequest {
    id: McpId,
    method: []const u8,
    params_json: []const u8,
    is_notification: bool,
    allocated_method: ([]u8)? = null,
    allocated_id_str: ([]u8)? = null,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (m := _.allocated_method) {
            alloc.free(m);
            _.allocated_method = null;
        }
        if (s := _.allocated_id_str) {
            alloc.free(s);
            _.allocated_id_str = null;
        }
    }
}

pub struct McpResponse {
    id: McpId,
    result_json: ([]const u8)? = null,
    err: McpError? = null,
}

pub struct McpNotification {
    method: []const u8,
    params_json: []const u8 = "{}",
}

pub fn appendJsonString(list: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): !void {
    try list.append(alloc, '"');
    for (c of s) {
        switch (c) {
            '"' => try list.appendSlice(alloc, "\\\""),
            '\\' => try list.appendSlice(alloc, "\\\\"),
            '\n' => try list.appendSlice(alloc, "\\n"),
            '\r' => try list.appendSlice(alloc, "\\r"),
            '\t' => try list.appendSlice(alloc, "\\t"),
            0x08 => try list.appendSlice(alloc, "\\b"),
            0x0C => try list.appendSlice(alloc, "\\f"),
            else => {
                if (c < 0x20) {
                    const esc = try std.fmt.allocPrint(alloc, "\\u{x:0>4}", .{c});
                    defer alloc.free(esc);
                    try list.appendSlice(alloc, esc);
                } else {
                    try list.append(alloc, c);
                }
            },
        }
    }
    try list.append(alloc, '"');
}

pub fn escapeJsonString(alloc: std.mem.Allocator, s: []const u8): ![]u8 {
    let list = std.ArrayList(u8).empty;
    errdefer list.deinit(alloc);
    try appendJsonString(&list, alloc, s);
    return try list.toOwnedSlice(alloc);
}

fn skipWs(src: []const u8, start: usize): usize {
    let i = start;
    while (i < src.len) {
        const c = src[i];
        if (c == ' ' or c == '\t' or c == '\r' or c == '\n') {
            i += 1;
        } else {
            break;
        }
    }
    return i;
}

fn hexDigit(c: u8): !u16 {
    if (c >= '0' and c <= '9') {
        return @as(u16, c - '0');
    }
    if (c >= 'a' and c <= 'f') {
        return @as(u16, c - 'a' + 10);
    }
    if (c >= 'A' and c <= 'F') {
        return @as(u16, c - 'A' + 10);
    }
    return error.ParseError;
}

struct ParsedString {
    value: []u8,
    next_idx: usize,
}

fn parseJsonStringAt(alloc: std.mem.Allocator, src: []const u8, start: usize): !ParsedString {
    if (start >= src.len or src[start] != '"') {
        return error.ParseError;
    }
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);
    let i = start + 1;
    while (i < src.len) {
        const c = src[i];
        if (c == '"') {
            const owned = try out.toOwnedSlice(alloc);
            return ParsedString{ .value = owned, .next_idx = i + 1 };
        }
        if (c == '\\') {
            i += 1;
            if (i >= src.len) {
                return error.ParseError;
            }
            const esc = src[i];
            switch (esc) {
                '"' => try out.append(alloc, '"'),
                '\\' => try out.append(alloc, '\\'),
                '/' => try out.append(alloc, '/'),
                'n' => try out.append(alloc, '\n'),
                'r' => try out.append(alloc, '\r'),
                't' => try out.append(alloc, '\t'),
                'b' => try out.append(alloc, 0x08),
                'f' => try out.append(alloc, 0x0C),
                'u' => {
                    if (i + 4 >= src.len) {
                        return error.ParseError;
                    }
                    const d0 = try hexDigit(src[i + 1]);
                    const d1 = try hexDigit(src[i + 2]);
                    const d2 = try hexDigit(src[i + 3]);
                    const d3 = try hexDigit(src[i + 4]);
                    const codepoint: u21 = @as(u21, (d0 << 12) | (d1 << 8) | (d2 << 4) | d3);
                    let utf8_buf: [4]u8 = undefined;
                    const utf8_len = try std.unicode.utf8Encode(codepoint, &utf8_buf);
                    try out.appendSlice(alloc, utf8_buf[0..utf8_len]);
                    i += 4;
                },
                else => return error.ParseError,
            }
            i += 1;
        } else {
            if (c < 0x20) {
                return error.ParseError;
            }
            try out.append(alloc, c);
            i += 1;
        }
    }
    return error.ParseError;
}

fn scanJsonValueEnd(src: []const u8, start: usize): !usize {
    if (start >= src.len) {
        return error.ParseError;
    }
    const first = src[start];
    if (first == '"') {
        let i = start + 1;
        while (i < src.len) {
            if (src[i] == '\\') {
                i += 2;
                continue;
            }
            if (src[i] == '"') {
                return i + 1;
            }
            i += 1;
        }
        return error.ParseError;
    }
    if (first == '{' or first == '[') {
        let depth: usize = 0;
        let i = start;
        let in_str = false;
        while (i < src.len) {
            const c = src[i];
            if (in_str) {
                if (c == '\\') {
                    i += 2;
                    continue;
                }
                if (c == '"') {
                    in_str = false;
                }
                i += 1;
                continue;
            }
            if (c == '"') {
                in_str = true;
            } else if (c == '{' or c == '[') {
                depth += 1;
            } else if (c == '}' or c == ']') {
                if (depth == 0) {
                    return error.ParseError;
                }
                depth -= 1;
                if (depth == 0) {
                    return i + 1;
                }
            }
            i += 1;
        }
        return error.ParseError;
    }
    let i = start;
    while (i < src.len) {
        const c = src[i];
        if (c == ',' or c == '}' or c == ']' or c == ' ' or c == '\t' or c == '\r' or c == '\n') {
            break;
        }
        i += 1;
    }
    if (i == start) {
        return error.ParseError;
    }
    return i;
}

pub fn extractJsonRawField(obj_json: []const u8, target_key: []const u8): ([]const u8)? {
    let i = skipWs(obj_json, 0);
    if (i >= obj_json.len or obj_json[i] != '{') {
        return null;
    }
    i = skipWs(obj_json, i + 1);
    if (i < obj_json.len and obj_json[i] == '}') {
        return null;
    }
    while (i < obj_json.len) {
        i = skipWs(obj_json, i);
        if (i >= obj_json.len or obj_json[i] != '"') {
            return null;
        }
        const key_end = scanJsonValueEnd(obj_json, i) catch return null;
        const raw_key = obj_json[i + 1 .. key_end - 1];
        i = skipWs(obj_json, key_end);
        if (i >= obj_json.len or obj_json[i] != ':') {
            return null;
        }
        i = skipWs(obj_json, i + 1);
        const val_start = i;
        const val_end = scanJsonValueEnd(obj_json, val_start) catch return null;
        if (std.mem.eql(u8, raw_key, target_key)) {
            return obj_json[val_start..val_end];
        }
        i = skipWs(obj_json, val_end);
        if (i < obj_json.len and obj_json[i] == ',') {
            i += 1;
            continue;
        }
        if (i < obj_json.len and obj_json[i] == '}') {
            return null;
        }
        return null;
    }
    return null;
}

pub fn extractJsonStringField(alloc: std.mem.Allocator, obj_json: []const u8, target_key: []const u8): !([]u8)? {
    if (raw := extractJsonRawField(obj_json, target_key)) {
        if (raw.len < 2 or raw[0] != '"') {
            return null;
        }
        const parsed = try parseJsonStringAt(alloc, raw, 0);
        return parsed.value;
    }
    return null;
}

pub fn decodeRequest(alloc: std.mem.Allocator, json_bytes: []const u8): !McpRequest {
    let i = skipWs(json_bytes, 0);
    if (i >= json_bytes.len or json_bytes[i] != '{') {
        return error.ParseError;
    }
    i = skipWs(json_bytes, i + 1);
    if (i >= json_bytes.len) {
        return error.ParseError;
    }

    let has_jsonrpc = false;
    let valid_jsonrpc = false;
    let method_opt: ([]u8)? = null;
    errdefer if (m := method_opt) alloc.free(m);
    let id_val = McpId.none();
    let id_str_alloc: ([]u8)? = null;
    errdefer if (s := id_str_alloc) alloc.free(s);
    let has_id = false;
    let params_slice: []const u8 = "{}";

    if (json_bytes[i] != '}') {
        while (i < json_bytes.len) {
            i = skipWs(json_bytes, i);
            if (i >= json_bytes.len or json_bytes[i] != '"') {
                return error.ParseError;
            }
            const parsed_key = try parseJsonStringAt(alloc, json_bytes, i);
            defer alloc.free(parsed_key.value);
            i = skipWs(json_bytes, parsed_key.next_idx);
            if (i >= json_bytes.len or json_bytes[i] != ':') {
                return error.ParseError;
            }
            i = skipWs(json_bytes, i + 1);
            const val_start = i;
            const val_end = try scanJsonValueEnd(json_bytes, val_start);
            const val_raw = json_bytes[val_start..val_end];

            if (std.mem.eql(u8, parsed_key.value, "jsonrpc")) {
                has_jsonrpc = true;
                if (val_raw.len >= 2 and val_raw[0] == '"') {
                    const ver = try parseJsonStringAt(alloc, val_raw, 0);
                    defer alloc.free(ver.value);
                    if (std.mem.eql(u8, ver.value, "2.0")) {
                        valid_jsonrpc = true;
                    }
                }
            } else if (std.mem.eql(u8, parsed_key.value, "method")) {
                if (val_raw.len >= 2 and val_raw[0] == '"') {
                    const m = try parseJsonStringAt(alloc, val_raw, 0);
                    if (old := method_opt) alloc.free(old);
                    method_opt = m.value;
                } else {
                    return error.InvalidRequest;
                }
            } else if (std.mem.eql(u8, parsed_key.value, "id")) {
                has_id = true;
                if (val_raw.len >= 2 and val_raw[0] == '"') {
                    const s = try parseJsonStringAt(alloc, val_raw, 0);
                    if (old := id_str_alloc) alloc.free(old);
                    id_str_alloc = s.value;
                    id_val = McpId.fromString(s.value);
                } else if (std.mem.eql(u8, val_raw, "null")) {
                    id_val = McpId.none();
                } else {
                    const n = std.fmt.parseInt(i64, val_raw, 10) catch return error.InvalidRequest;
                    id_val = McpId.fromInt(n);
                }
            } else if (std.mem.eql(u8, parsed_key.value, "params")) {
                params_slice = val_raw;
            }

            i = skipWs(json_bytes, val_end);
            if (i < json_bytes.len and json_bytes[i] == ',') {
                i += 1;
                continue;
            }
            if (i < json_bytes.len and json_bytes[i] == '}') {
                i += 1;
                break;
            }
            return error.ParseError;
        }
    } else {
        i += 1;
    }

    const tail = skipWs(json_bytes, i);
    if (tail != json_bytes.len) {
        return error.ParseError;
    }
    if (!has_jsonrpc or !valid_jsonrpc) {
        return error.InvalidRequest;
    }
    const method = method_opt orelse return error.InvalidRequest;
    if (method.len == 0) {
        return error.InvalidRequest;
    }

    return McpRequest{
        .id = id_val,
        .method = method,
        .params_json = params_slice,
        .is_notification = !has_id,
        .allocated_method = method,
        .allocated_id_str = id_str_alloc,
    };
}

pub fn encodeResponse(alloc: std.mem.Allocator, resp: McpResponse): ![]u8 {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    try out.appendSlice(alloc, "{\"jsonrpc\":\"2.0\",\"id\":");
    try resp.id.appendJson(&out, alloc);
    if (e := resp.err) {
        try out.appendSlice(alloc, ",\"error\":{\"code\":");
        const code_s = try std.fmt.allocPrint(alloc, "{d}", .{e.code});
        defer alloc.free(code_s);
        try out.appendSlice(alloc, code_s);
        try out.appendSlice(alloc, ",\"message\":");
        try appendJsonString(&out, alloc, e.message);
        if (d := e.data) {
            try out.appendSlice(alloc, ",\"data\":");
            try appendJsonString(&out, alloc, d);
        }
        try out.appendSlice(alloc, "}}");
    } else {
        try out.appendSlice(alloc, ",\"result\":");
        try out.appendSlice(alloc, resp.result_json orelse "null");
        try out.append(alloc, '}');
    }
    return try out.toOwnedSlice(alloc);
}

pub fn encodeErrorResponse(
    alloc: std.mem.Allocator,
    id: McpId,
    code: i32,
    message: []const u8,
    data: ([]const u8)?,
): ![]u8 {
    return try encodeResponse(alloc, McpResponse{
        .id = id,
        .result_json = null,
        .err = McpError{
            .code = code,
            .message = message,
            .data = data,
        },
    });
}

pub fn encodeNotification(alloc: std.mem.Allocator, notif: McpNotification): ![]u8 {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    try out.appendSlice(alloc, "{\"jsonrpc\":\"2.0\",\"method\":");
    try appendJsonString(&out, alloc, notif.method);
    try out.appendSlice(alloc, ",\"params\":");
    try out.appendSlice(alloc, notif.params_json);
    try out.append(alloc, '}');
    return try out.toOwnedSlice(alloc);
}
