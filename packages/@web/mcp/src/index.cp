// Official @web/mcp Plugin — Model Context Protocol (MCP) Streamable HTTP JSON-RPC 2.0 Server for cap:web
// Pure CP: zero zig{}, zero var, zero anyopaque, zero heap GC.

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";

pub const MCP_MAX_TOOLS: usize = 64;
pub const MCP_RESP_BUF_CAP: usize = 16384;
pub const MCP_STR_BUF_CAP: usize = 1024;

pub struct McpConfig {
    endpoint_path: string = "/mcp",
    server_name: string = "cp-web-mcp",
    server_version: string = "1.0.0",
}

pub type McpToolHandlerFn = *const fn (*Context, string): string;

struct McpToolEntry {
    name: string = "",
    description: string = "",
    handler: McpToolHandlerFn? = null,
}

let mcp_cfg: McpConfig = McpConfig{};
let mcp_tools: [MCP_MAX_TOOLS]McpToolEntry = [McpToolEntry{}; MCP_MAX_TOOLS];
let mcp_tool_len: usize = 0;
let mcp_resp_buf: [MCP_RESP_BUF_CAP]u8 = [0; MCP_RESP_BUF_CAP];
let mcp_method_buf: [MCP_STR_BUF_CAP]u8 = [0; MCP_STR_BUF_CAP];
let mcp_tool_name_buf: [MCP_STR_BUF_CAP]u8 = [0; MCP_STR_BUF_CAP];

fn mcpBytesEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn mcpWriteByte(out_buf: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn mcpWriteRaw(out_buf: []u8, pos: *usize, s: string): bool {
    if (s.len > out_buf.len || pos.* > out_buf.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn mcpHexLower(n: u8): u8 {
    return if (n < 10) ('0' + n) else ('a' + (n - 10));
}

fn mcpWriteJsonEscaped(out_buf: []u8, pos: *usize, s: string): bool {
    if (!mcpWriteByte(out_buf, pos, '"')) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (b == '"') {
            if (!mcpWriteRaw(out_buf, pos, "\\\"")) return false;
        } else if (b == '\\') {
            if (!mcpWriteRaw(out_buf, pos, "\\\\")) return false;
        } else if (b == '\n') {
            if (!mcpWriteRaw(out_buf, pos, "\\n")) return false;
        } else if (b == '\r') {
            if (!mcpWriteRaw(out_buf, pos, "\\r")) return false;
        } else if (b == '\t') {
            if (!mcpWriteRaw(out_buf, pos, "\\t")) return false;
        } else if (b < 0x20) {
            if (!mcpWriteRaw(out_buf, pos, "\\u00")) return false;
            if (!mcpWriteByte(out_buf, pos, mcpHexLower((b >> 4) & 0x0F))) return false;
            if (!mcpWriteByte(out_buf, pos, mcpHexLower(b & 0x0F))) return false;
        } else {
            if (!mcpWriteByte(out_buf, pos, b)) return false;
        }
    }
    return mcpWriteByte(out_buf, pos, '"');
}

fn mcpSkipWs(s: string, start: usize): usize {
    let i: usize = start;
    while (i < s.len && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) : (i += 1) {}
    return i;
}

/// Skip a JSON value starting at `start`, returning the index right after the value.
fn mcpSkipJsonValue(s: string, start: usize): usize {
    let i = mcpSkipWs(s, start);
    if (i >= s.len) return i;
    const c = s[i];
    if (c == '"') {
        i += 1;
        while (i < s.len) {
            if (s[i] == '\\' && i + 1 < s.len) {
                i += 2;
                continue;
            }
            if (s[i] == '"') {
                return i + 1;
            }
            i += 1;
        }
        return s.len;
    }
    if (c == '{' || c == '[') {
        const open_ch = c;
        const close_ch: u8 = if (c == '{') '}' else ']';
        let depth: usize = 1;
        i += 1;
        while (i < s.len && depth > 0) {
            if (s[i] == '"') {
                i += 1;
                while (i < s.len) {
                    if (s[i] == '\\' && i + 1 < s.len) {
                        i += 2;
                        continue;
                    }
                    if (s[i] == '"') {
                        i += 1;
                        break;
                    }
                    i += 1;
                }
                continue;
            }
            if (s[i] == open_ch) {
                depth += 1;
            } else if (s[i] == close_ch) {
                depth -= 1;
            }
            i += 1;
        }
        return i;
    }
    // Primitive (number, true, false, null)
    while (i < s.len) {
        const ch = s[i];
        if (ch == ',' || ch == '}' || ch == ']' || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            break;
        }
        i += 1;
    }
    return i;
}

/// Find the raw JSON value slice for a top-level key inside a JSON object string `obj`.
fn mcpFindObjectFieldRaw(obj: string, target_key: string): string? {
    let i = mcpSkipWs(obj, 0);
    if (i >= obj.len || obj[i] != '{') return null;
    i += 1;
    while (i < obj.len) {
        i = mcpSkipWs(obj, i);
        if (i >= obj.len || obj[i] == '}') return null;
        if (obj[i] == ',') {
            i += 1;
            continue;
        }
        if (obj[i] != '"') return null;
        const key_start = i + 1;
        i += 1;
        while (i < obj.len) {
            if (obj[i] == '\\' && i + 1 < obj.len) {
                i += 2;
                continue;
            }
            if (obj[i] == '"') break;
            i += 1;
        }
        if (i >= obj.len) return null;
        const key = obj[key_start..i];
        i = mcpSkipWs(obj, i + 1);
        if (i >= obj.len || obj[i] != ':') return null;
        const val_start = mcpSkipWs(obj, i + 1);
        const val_end = mcpSkipJsonValue(obj, val_start);
        if (mcpBytesEql(key, target_key)) {
            return obj[val_start..val_end];
        }
        i = val_end;
    }
    return null;
}

/// Unescape a raw JSON string token (including surrounding quotes) into `out_buf`.
fn mcpUnescapeJsonString(raw: string, out_buf: []u8): usize? {
    if (raw.len < 2 || raw[0] != '"' || raw[raw.len - 1] != '"') return null;
    let p: usize = 1;
    let out_pos: usize = 0;
    const end = raw.len - 1;
    while (p < end) {
        const ch = raw[p];
        if (ch == '\\' && p + 1 < end) {
            const nxt = raw[p + 1];
            if (nxt == '"') {
                if (!mcpWriteByte(out_buf, *out_pos, '"')) return null;
            } else if (nxt == '\\') {
                if (!mcpWriteByte(out_buf, *out_pos, '\\')) return null;
            } else if (nxt == 'n') {
                if (!mcpWriteByte(out_buf, *out_pos, '\n')) return null;
            } else if (nxt == 'r') {
                if (!mcpWriteByte(out_buf, *out_pos, '\r')) return null;
            } else if (nxt == 't') {
                if (!mcpWriteByte(out_buf, *out_pos, '\t')) return null;
            } else {
                if (!mcpWriteByte(out_buf, *out_pos, nxt)) return null;
            }
            p += 2;
            continue;
        }
        if (!mcpWriteByte(out_buf, *out_pos, ch)) return null;
        p += 1;
    }
    return out_pos;
}

/// Register an MCP tool. Overwrites existing tool with the same `name`.
pub fn mcpRegisterTool(name: string, description: string, handler: McpToolHandlerFn): bool {
    if (name.len == 0) return false;
    let i: usize = 0;
    while (i < mcp_tool_len) : (i += 1) {
        if (mcpBytesEql(mcp_tools[i].name, name)) {
            mcp_tools[i].description = description;
            mcp_tools[i].handler = handler;
            return true;
        }
    }
    if (mcp_tool_len >= MCP_MAX_TOOLS) return false;
    mcp_tools[mcp_tool_len] = McpToolEntry{
        name: name,
        description: description,
        handler: handler,
    };
    mcp_tool_len += 1;
    return true;
}

/// Clear all registered MCP tools.
pub fn mcpResetTools() {
    mcp_tool_len = 0;
}

fn mcpFindTool(name: string): McpToolHandlerFn? {
    let i: usize = 0;
    while (i < mcp_tool_len) : (i += 1) {
        if (mcpBytesEql(mcp_tools[i].name, name)) {
            return mcp_tools[i].handler;
        }
    }
    return null;
}

fn mcpJsonRpcError(id_raw: string, code_str: string, message: string): Response {
    let pos: usize = 0;
    if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, "{\"jsonrpc\":\"2.0\",\"id\":")) {
        return Response.json(500, "{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":-32603,\"message\":\"Internal error\"}}");
    }
    if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, id_raw)) {
        return Response.json(500, "{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":-32603,\"message\":\"Internal error\"}}");
    }
    if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, ",\"error\":{\"code\":")) {
        return Response.json(500, "{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":-32603,\"message\":\"Internal error\"}}");
    }
    if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, code_str)) {
        return Response.json(500, "{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":-32603,\"message\":\"Internal error\"}}");
    }
    if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, ",\"message\":")) {
        return Response.json(500, "{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":-32603,\"message\":\"Internal error\"}}");
    }
    if (!mcpWriteJsonEscaped(mcp_resp_buf[0..], *pos, message)) {
        return Response.json(500, "{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":-32603,\"message\":\"Internal error\"}}");
    }
    if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, "}}")) {
        return Response.json(500, "{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":-32603,\"message\":\"Internal error\"}}");
    }
    return Response.json(200, mcp_resp_buf[0..pos]);
}

fn mcpPostHandler(ctx: *Context): Response {
    let id_raw: string = "null";
    if (found_id := mcpFindObjectFieldRaw(ctx.body, "id")) {
        if (found_id.len > 0) {
            id_raw = found_id;
        }
    }

    let method_len: usize = 0;
    if (method_raw := mcpFindObjectFieldRaw(ctx.body, "method")) {
        if (n := mcpUnescapeJsonString(method_raw, mcp_method_buf[0..])) {
            method_len = n;
        } else {
            return mcpJsonRpcError(id_raw, "-32600", "Invalid Request");
        }
    } else {
        return mcpJsonRpcError(id_raw, "-32600", "Invalid Request");
    }

    const method = mcp_method_buf[0..method_len];

    if (mcpBytesEql(method, "initialize")) {
        let pos: usize = 0;
        if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, "{\"jsonrpc\":\"2.0\",\"id\":")) {
            return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
        }
        if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, id_raw)) {
            return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
        }
        if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, ",\"result\":{\"protocolVersion\":\"2025-03-26\",\"serverInfo\":{\"name\":")) {
            return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
        }
        if (!mcpWriteJsonEscaped(mcp_resp_buf[0..], *pos, mcp_cfg.server_name)) {
            return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
        }
        if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, ",\"version\":")) {
            return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
        }
        if (!mcpWriteJsonEscaped(mcp_resp_buf[0..], *pos, mcp_cfg.server_version)) {
            return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
        }
        if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, "},\"capabilities\":{\"tools\":{}}}}")) {
            return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
        }
        return Response.json(200, mcp_resp_buf[0..pos]);
    }

    if (mcpBytesEql(method, "tools/list")) {
        let pos: usize = 0;
        if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, "{\"jsonrpc\":\"2.0\",\"id\":")) {
            return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
        }
        if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, id_raw)) {
            return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
        }
        if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, ",\"result\":{\"tools\":[")) {
            return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
        }
        let i: usize = 0;
        while (i < mcp_tool_len) : (i += 1) {
            if (i > 0) {
                if (!mcpWriteByte(mcp_resp_buf[0..], *pos, ',')) {
                    return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
                }
            }
            if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, "{\"name\":")) {
                return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
            }
            if (!mcpWriteJsonEscaped(mcp_resp_buf[0..], *pos, mcp_tools[i].name)) {
                return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
            }
            if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, ",\"description\":")) {
                return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
            }
            if (!mcpWriteJsonEscaped(mcp_resp_buf[0..], *pos, mcp_tools[i].description)) {
                return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
            }
            if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, ",\"inputSchema\":{\"type\":\"object\"}}")) {
                return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
            }
        }
        if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, "]}}")) {
            return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
        }
        return Response.json(200, mcp_resp_buf[0..pos]);
    }

    if (mcpBytesEql(method, "tools/call")) {
        let params_raw: string = "{}";
        if (p_raw := mcpFindObjectFieldRaw(ctx.body, "params")) {
            params_raw = p_raw;
        }
        let tool_name_len: usize = 0;
        if (name_raw := mcpFindObjectFieldRaw(params_raw, "name")) {
            if (n := mcpUnescapeJsonString(name_raw, mcp_tool_name_buf[0..])) {
                tool_name_len = n;
            } else {
                return mcpJsonRpcError(id_raw, "-32602", "Invalid params: name");
            }
        } else {
            return mcpJsonRpcError(id_raw, "-32602", "Invalid params: missing name");
        }

        let args_raw: string = "{}";
        if (a_raw := mcpFindObjectFieldRaw(params_raw, "arguments")) {
            args_raw = a_raw;
        }

        const tool_name = mcp_tool_name_buf[0..tool_name_len];
        if (tool_fn := mcpFindTool(tool_name)) {
            const out_text = tool_fn(ctx, args_raw);
            let pos: usize = 0;
            if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, "{\"jsonrpc\":\"2.0\",\"id\":")) {
                return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
            }
            if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, id_raw)) {
                return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
            }
            if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, ",\"result\":{\"content\":[{\"type\":\"text\",\"text\":")) {
                return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
            }
            if (!mcpWriteJsonEscaped(mcp_resp_buf[0..], *pos, out_text)) {
                return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
            }
            if (!mcpWriteRaw(mcp_resp_buf[0..], *pos, "}]}}")) {
                return mcpJsonRpcError(id_raw, "-32603", "Buffer overflow");
            }
            return Response.json(200, mcp_resp_buf[0..pos]);
        }
        return mcpJsonRpcError(id_raw, "-32601", "Tool not found");
    }

    return mcpJsonRpcError(id_raw, "-32601", "Method not found");
}

/// Register the MCP Streamable HTTP JSON-RPC 2.0 endpoint (`POST cfg.endpoint_path`) on `app`.
pub fn registerMcp(app: *App, cfg: McpConfig): *App {
    mcp_cfg = cfg;
    let p = Plugin.init("@web/mcp");
    * = p.withScope(PluginScope.Global);
    * = p.route(AppMethod.POST, cfg.endpoint_path, "MCP JSON-RPC 2.0", &mcpPostHandler);
    return app.usePlugin(&p);
}
