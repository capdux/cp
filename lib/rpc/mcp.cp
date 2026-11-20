// Source: cp/lib/rpc/mcp.cp (Model Context Protocol Core Server Dispatcher)
// Pure CP Model Context Protocol (MCP) server dispatcher supporting initialize,
// notifications/initialized, ping, tools/list, tools/call & resources/list.

const std = @import("std");
import { JsonValue, JsonParser, JsonSerializer } from "cap:json";
import {
    ERR_PARSE,
    ERR_INVALID_REQUEST,
    ERR_METHOD_NOT_FOUND,
    ERR_INVALID_PARAMS,
    ERR_INTERNAL,
    JsonRpcId,
    JsonRpcError,
    JsonRpcRequest,
    JsonRpcCodec,
} from "./jsonrpc.cp";

pub type McpToolHandler = fn (alloc: std.mem.Allocator, args_json: []const u8): ([]const u8)!;

pub struct McpToolSpec {
    name: []const u8 = "",
    description: []const u8 = "",
    input_schema_json: []const u8 = "{\"type\":\"object\",\"properties\":{}}",
}

pub struct McpResourceSpec {
    uri: []const u8 = "",
    name: []const u8 = "",
    mime_type: []const u8 = "text/plain",
    description: []const u8 = "",
}

pub struct McpServerInfo {
    name: []const u8 = "cp-mcp-server",
    version: []const u8 = "1.0.0",
    protocol_version: []const u8 = "2024-11-05",
}

struct RegisteredTool {
    spec: McpToolSpec = McpToolSpec{},
    handler: McpToolHandler = undefined,

    fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.spec.name.len > 0) alloc.free(_.spec.name);
        if (_.spec.description.len > 0) alloc.free(_.spec.description);
        if (_.spec.input_schema_json.len > 0) alloc.free(_.spec.input_schema_json);
    }
}

struct RegisteredResource {
    spec: McpResourceSpec = McpResourceSpec{},

    fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.spec.uri.len > 0) alloc.free(_.spec.uri);
        if (_.spec.name.len > 0) alloc.free(_.spec.name);
        if (_.spec.mime_type.len > 0) alloc.free(_.spec.mime_type);
        if (_.spec.description.len > 0) alloc.free(_.spec.description);
    }
}

pub struct McpServer {
    allocator: std.mem.Allocator = undefined,
    info: McpServerInfo = McpServerInfo{},
    tools: std.ArrayList(RegisteredTool) = .empty,
    resources: std.ArrayList(RegisteredResource) = .empty,
    initialized: bool = false,

    pub static fn init(alloc: std.mem.Allocator, info: McpServerInfo = McpServerInfo{}): McpServer {
        return McpServer{
            allocator: alloc,
            info: info,
            tools: .empty,
            resources: .empty,
            initialized: false,
        };
    }

    pub fn deinit(*_) {
        for (*t of _.tools.items) {
            t.deinit(_.allocator);
        }
        _.tools.deinit(_.allocator);

        for (*r of _.resources.items) {
            r.deinit(_.allocator);
        }
        _.resources.deinit(_.allocator);
    }

    pub fn toolCount(): usize {
        return _.tools.items.len;
    }

    pub fn resourceCount(): usize {
        return _.resources.items.len;
    }

    pub fn isInitialized(): bool {
        return _.initialized;
    }

    pub fn registerTool(*_, spec: McpToolSpec, handler_fn: McpToolHandler): ! {
        const owned_name = _.allocator.dupe(u8, spec.name)?;
        errdefer _.allocator.free(owned_name);

        const owned_desc = _.allocator.dupe(u8, spec.description)?;
        errdefer _.allocator.free(owned_desc);

        const schema_src = if (spec.input_schema_json.len > 0) spec.input_schema_json else "{\"type\":\"object\",\"properties\":{}}";
        const owned_schema = _.allocator.dupe(u8, schema_src)?;
        errdefer _.allocator.free(owned_schema);

        for (*existing of _.tools.items) {
            if (std.mem.eql(u8, existing.spec.name, spec.name)) {
                existing.deinit(_.allocator);
                existing.spec = McpToolSpec{
                    name: owned_name,
                    description: owned_desc,
                    input_schema_json: owned_schema,
                };
                existing.handler = handler_fn;
                return;
            }
        }

        _.tools.append(_.allocator, RegisteredTool{
            spec: McpToolSpec{
                name: owned_name,
                description: owned_desc,
                input_schema_json: owned_schema,
            },
            handler: handler_fn,
        })?;
    }

    pub fn registerResource(*_, spec: McpResourceSpec): ! {
        const owned_uri = _.allocator.dupe(u8, spec.uri)?;
        errdefer _.allocator.free(owned_uri);

        const owned_name = _.allocator.dupe(u8, spec.name)?;
        errdefer _.allocator.free(owned_name);

        const mime_src = if (spec.mime_type.len > 0) spec.mime_type else "text/plain";
        const owned_mime = _.allocator.dupe(u8, mime_src)?;
        errdefer _.allocator.free(owned_mime);

        const owned_desc = _.allocator.dupe(u8, spec.description)?;
        errdefer _.allocator.free(owned_desc);

        for (*existing of _.resources.items) {
            if (std.mem.eql(u8, existing.spec.uri, spec.uri)) {
                existing.deinit(_.allocator);
                existing.spec = McpResourceSpec{
                    uri: owned_uri,
                    name: owned_name,
                    mime_type: owned_mime,
                    description: owned_desc,
                };
                return;
            }
        }

        _.resources.append(_.allocator, RegisteredResource{
            spec: McpResourceSpec{
                uri: owned_uri,
                name: owned_name,
                mime_type: owned_mime,
                description: owned_desc,
            },
        })?;
    }

    fn buildInitializeResult(alloc: std.mem.Allocator): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(alloc, "{\"protocolVersion\":")?;
        JsonRpcCodec.appendEscapedJsonString(&out, alloc, _.info.protocol_version)?;
        out.appendSlice(alloc, ",\"capabilities\":{\"tools\":{\"listChanged\":false},\"resources\":{\"subscribe\":false,\"listChanged\":false}},\"serverInfo\":{\"name\":")?;
        JsonRpcCodec.appendEscapedJsonString(&out, alloc, _.info.name)?;
        out.appendSlice(alloc, ",\"version\":")?;
        JsonRpcCodec.appendEscapedJsonString(&out, alloc, _.info.version)?;
        out.appendSlice(alloc, "}}")?;
        return out.toOwnedSlice(alloc);
    }

    fn buildToolsListResult(alloc: std.mem.Allocator): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(alloc, "{\"tools\":[")?;
        let i: usize = 0;
        while (i < _.tools.items.len) : (i += 1) {
            if (i > 0) out.append(alloc, ',')?;
            const t = _.tools.items[i].spec;
            out.appendSlice(alloc, "{\"name\":")?;
            JsonRpcCodec.appendEscapedJsonString(&out, alloc, t.name)?;
            out.appendSlice(alloc, ",\"description\":")?;
            JsonRpcCodec.appendEscapedJsonString(&out, alloc, t.description)?;
            out.appendSlice(alloc, ",\"inputSchema\":")?;
            const trimmed_schema = std.mem.trim(u8, t.input_schema_json, " \t\r\n");
            if (trimmed_schema.len > 0) {
                out.appendSlice(alloc, trimmed_schema)?;
            } else {
                out.appendSlice(alloc, "{\"type\":\"object\",\"properties\":{}}")?;
            }
            out.append(alloc, '}')?;
        }
        out.appendSlice(alloc, "]}")?;
        return out.toOwnedSlice(alloc);
    }

    fn buildResourcesListResult(alloc: std.mem.Allocator): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(alloc, "{\"resources\":[")?;
        let i: usize = 0;
        while (i < _.resources.items.len) : (i += 1) {
            if (i > 0) out.append(alloc, ',')?;
            const r = _.resources.items[i].spec;
            out.appendSlice(alloc, "{\"uri\":")?;
            JsonRpcCodec.appendEscapedJsonString(&out, alloc, r.uri)?;
            out.appendSlice(alloc, ",\"name\":")?;
            JsonRpcCodec.appendEscapedJsonString(&out, alloc, r.name)?;
            out.appendSlice(alloc, ",\"mimeType\":")?;
            JsonRpcCodec.appendEscapedJsonString(&out, alloc, r.mime_type)?;
            if (r.description.len > 0) {
                out.appendSlice(alloc, ",\"description\":")?;
                JsonRpcCodec.appendEscapedJsonString(&out, alloc, r.description)?;
            }
            out.append(alloc, '}')?;
        }
        out.appendSlice(alloc, "]}")?;
        return out.toOwnedSlice(alloc);
    }

    static fn buildToolCallResult(alloc: std.mem.Allocator, text: []const u8, is_error: bool): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(alloc, "{\"content\":[{\"type\":\"text\",\"text\":")?;
        JsonRpcCodec.appendEscapedJsonString(&out, alloc, text)?;
        if (is_error) {
            out.appendSlice(alloc, "}],\"isError\":true}")?;
        } else {
            out.appendSlice(alloc, "}],\"isError\":false}")?;
        }
        return out.toOwnedSlice(alloc);
    }

    fn findTool(name: []const u8): McpToolHandler? {
        for (t of _.tools.items) {
            if (std.mem.eql(u8, t.spec.name, name)) {
                return t.handler;
            }
        }
        return null;
    }

    pub fn handleRequestJson(*_, alloc: std.mem.Allocator, req_json: []const u8): ([]u8)?! {
        let tmp_arena = std.heap.ArenaAllocator.init(alloc);
        defer tmp_arena.deinit();
        const arena_alloc = tmp_arena.allocator();

        const req = JsonRpcCodec.parseRequest(arena_alloc, req_json) catch e => {
            if (e == @as(anyerror, err.ParseError)) {
                const encoded = JsonRpcCodec.encodeError(alloc, JsonRpcId.Null, JsonRpcError.parseError())?;
                return encoded;
            }
            const encoded = JsonRpcCodec.encodeError(alloc, JsonRpcId.Null, JsonRpcError.invalidRequest())?;
            return encoded;
        };

        if (req.isNotification()) {
            if (std.mem.eql(u8, req.method, "notifications/initialized")) {
                _.initialized = true;
            }
            return null;
        }

        if (std.mem.eql(u8, req.method, "initialize")) {
            const res_json = _.buildInitializeResult(arena_alloc)?;
            const encoded = JsonRpcCodec.encodeSuccess(alloc, req.id, res_json)?;
            return encoded;
        }

        if (std.mem.eql(u8, req.method, "ping")) {
            const encoded = JsonRpcCodec.encodeSuccess(alloc, req.id, "{}")?;
            return encoded;
        }

        if (std.mem.eql(u8, req.method, "tools/list")) {
            const res_json = _.buildToolsListResult(arena_alloc)?;
            const encoded = JsonRpcCodec.encodeSuccess(alloc, req.id, res_json)?;
            return encoded;
        }

        if (std.mem.eql(u8, req.method, "resources/list")) {
            const res_json = _.buildResourcesListResult(arena_alloc)?;
            const encoded = JsonRpcCodec.encodeSuccess(alloc, req.id, res_json)?;
            return encoded;
        }

        if (std.mem.eql(u8, req.method, "tools/call")) {
            if (req.params_json.len == 0) {
                const encoded = JsonRpcCodec.encodeError(
                    alloc,
                    req.id,
                    JsonRpcError.init(ERR_INVALID_PARAMS, "Missing params for tools/call"),
                )?;
                return encoded;
            }
            const params_val = JsonParser.parse(arena_alloc, req.params_json) catch {
                const encoded = JsonRpcCodec.encodeError(alloc, req.id, JsonRpcError.invalidParams())?;
                return encoded;
            };
            if (!params_val.isObject()) {
                const encoded = JsonRpcCodec.encodeError(alloc, req.id, JsonRpcError.invalidParams())?;
                return encoded;
            }
            const name_val = params_val.get("name") ?? {
                const encoded = JsonRpcCodec.encodeError(
                    alloc,
                    req.id,
                    JsonRpcError.init(ERR_INVALID_PARAMS, "Missing tool name"),
                )?;
                return encoded;
            };
            const tool_name = name_val.asString() ?? {
                const encoded = JsonRpcCodec.encodeError(
                    alloc,
                    req.id,
                    JsonRpcError.init(ERR_INVALID_PARAMS, "Tool name must be a string"),
                )?;
                return encoded;
            };

            if (handler := _.findTool(tool_name)) {
                let args_json: []const u8 = "{}";
                if (args_val := params_val.get("arguments")) {
                    if (!args_val.isObject()) {
                        const encoded = JsonRpcCodec.encodeError(
                            alloc,
                            req.id,
                            JsonRpcError.init(ERR_INVALID_PARAMS, "Tool arguments must be an object"),
                        )?;
                        return encoded;
                    }
                    args_json = JsonSerializer.stringify(arena_alloc, args_val)?;
                }

                const tool_out = handler(arena_alloc, args_json) catch tool_err => {
                    let err_msg: []const u8 = "Tool execution failed";
                    zig {
                        err_msg = @errorName(tool_err);
                    }
                    const err_result = McpServer.buildToolCallResult(arena_alloc, err_msg, true)?;
                    const encoded = JsonRpcCodec.encodeSuccess(alloc, req.id, err_result)?;
                    return encoded;
                };

                const ok_result = McpServer.buildToolCallResult(arena_alloc, tool_out, false)?;
                const encoded = JsonRpcCodec.encodeSuccess(alloc, req.id, ok_result)?;
                return encoded;
            } else {
                const encoded = JsonRpcCodec.encodeError(
                    alloc,
                    req.id,
                    JsonRpcError.init(ERR_INVALID_PARAMS, "Unknown tool"),
                )?;
                return encoded;
            }
        }

        const encoded = JsonRpcCodec.encodeError(alloc, req.id, JsonRpcError.methodNotFound())?;
        return encoded;
    }
}
