// Source: cp/lib/mcp/server.cp (MCP 2025-03-26 JSON-RPC 2.0 router and capability server)
const std = @import("std");
const proto = @import("./protocol.cp");

pub const ToolHandler = *const fn (alloc: std.mem.Allocator, arguments_json: []const u8) anyerror!proto.McpToolResult;
pub const ResourceReader = *const fn (alloc: std.mem.Allocator, uri: []const u8) anyerror![]u8;
pub const PromptBuilder = *const fn (alloc: std.mem.Allocator, arguments_json: []const u8) anyerror![]u8;

struct RegisteredTool {
    def: proto.McpToolDef,
    handler: ToolHandler,
}

struct RegisteredResource {
    def: proto.McpResourceDef,
    reader: ResourceReader,
}

struct RegisteredPrompt {
    def: proto.McpPromptDef,
    builder: PromptBuilder,
}

pub struct McpServer {
    alloc: std.mem.Allocator,
    name: []const u8,
    version: []const u8,
    initialized: bool,
    tools: std.ArrayList(RegisteredTool),
    resources: std.ArrayList(RegisteredResource),
    prompts: std.ArrayList(RegisteredPrompt),

    pub static fn init(alloc: std.mem.Allocator, name: []const u8, version: []const u8): McpServer {
        return McpServer{
            .alloc = alloc,
            .name = name,
            .version = version,
            .initialized = false,
            .tools = std.ArrayList(RegisteredTool).empty,
            .resources = std.ArrayList(RegisteredResource).empty,
            .prompts = std.ArrayList(RegisteredPrompt).empty,
        };
    }

    pub fn deinit(*_) {
        _.tools.deinit(_.alloc);
        _.resources.deinit(_.alloc);
        _.prompts.deinit(_.alloc);
    }

    pub fn registerTool(*_, def: proto.McpToolDef, handler: ToolHandler): !void {
        try _.tools.append(_.alloc, RegisteredTool{ .def = def, .handler = handler });
    }

    pub fn registerResource(*_, def: proto.McpResourceDef, reader: ResourceReader): !void {
        try _.resources.append(_.alloc, RegisteredResource{ .def = def, .reader = reader });
    }

    pub fn registerPrompt(*_, def: proto.McpPromptDef, builder: PromptBuilder): !void {
        try _.prompts.append(_.alloc, RegisteredPrompt{ .def = def, .builder = builder });
    }

    fn buildInitializeResult(): ![]u8 {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(_.alloc);

        try out.appendSlice(_.alloc, "{\"protocolVersion\":");
        try proto.appendJsonString(&out, _.alloc, proto.MCP_PROTOCOL_VERSION);
        try out.appendSlice(_.alloc, ",\"capabilities\":{");
        let need_comma = false;
        if (_.tools.items.len > 0) {
            try out.appendSlice(_.alloc, "\"tools\":{}");
            need_comma = true;
        }
        if (_.resources.items.len > 0) {
            if (need_comma) try out.append(_.alloc, ',');
            try out.appendSlice(_.alloc, "\"resources\":{}");
            need_comma = true;
        }
        if (_.prompts.items.len > 0) {
            if (need_comma) try out.append(_.alloc, ',');
            try out.appendSlice(_.alloc, "\"prompts\":{}");
        }
        try out.appendSlice(_.alloc, "},\"serverInfo\":{\"name\":");
        try proto.appendJsonString(&out, _.alloc, _.name);
        try out.appendSlice(_.alloc, ",\"version\":");
        try proto.appendJsonString(&out, _.alloc, _.version);
        try out.appendSlice(_.alloc, "}}");
        return try out.toOwnedSlice(_.alloc);
    }

    fn buildToolsListResult(): ![]u8 {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(_.alloc);

        try out.appendSlice(_.alloc, "{\"tools\":[");
        let idx: usize = 0;
        while (idx < _.tools.items.len) {
            const t = _.tools.items[idx];
            if (idx > 0) try out.append(_.alloc, ',');
            try out.appendSlice(_.alloc, "{\"name\":");
            try proto.appendJsonString(&out, _.alloc, t.def.name);
            try out.appendSlice(_.alloc, ",\"description\":");
            try proto.appendJsonString(&out, _.alloc, t.def.description);
            try out.appendSlice(_.alloc, ",\"inputSchema\":");
            try out.appendSlice(_.alloc, t.def.input_schema_json);
            try out.append(_.alloc, '}');
            idx += 1;
        }
        try out.appendSlice(_.alloc, "]}");
        return try out.toOwnedSlice(_.alloc);
    }

    fn encodeToolResult(res: proto.McpToolResult): ![]u8 {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(_.alloc);

        try out.appendSlice(_.alloc, "{\"content\":[");
        let idx: usize = 0;
        while (idx < res.content.len) {
            const item = res.content[idx];
            if (idx > 0) try out.append(_.alloc, ',');
            try out.appendSlice(_.alloc, "{\"type\":");
            try proto.appendJsonString(&out, _.alloc, item.content_type);
            try out.appendSlice(_.alloc, ",\"text\":");
            try proto.appendJsonString(&out, _.alloc, item.text);
            if (mt := item.mime_type) {
                try out.appendSlice(_.alloc, ",\"mimeType\":");
                try proto.appendJsonString(&out, _.alloc, mt);
            }
            try out.append(_.alloc, '}');
            idx += 1;
        }
        try out.appendSlice(_.alloc, "],\"isError\":");
        try out.appendSlice(_.alloc, if (res.is_error) "true" else "false");
        try out.append(_.alloc, '}');
        return try out.toOwnedSlice(_.alloc);
    }

    fn buildResourcesListResult(): ![]u8 {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(_.alloc);

        try out.appendSlice(_.alloc, "{\"resources\":[");
        let idx: usize = 0;
        while (idx < _.resources.items.len) {
            const r = _.resources.items[idx];
            if (idx > 0) try out.append(_.alloc, ',');
            try out.appendSlice(_.alloc, "{\"uri\":");
            try proto.appendJsonString(&out, _.alloc, r.def.uri);
            try out.appendSlice(_.alloc, ",\"name\":");
            try proto.appendJsonString(&out, _.alloc, r.def.name);
            try out.appendSlice(_.alloc, ",\"description\":");
            try proto.appendJsonString(&out, _.alloc, r.def.description);
            try out.appendSlice(_.alloc, ",\"mimeType\":");
            try proto.appendJsonString(&out, _.alloc, r.def.mime_type);
            try out.append(_.alloc, '}');
            idx += 1;
        }
        try out.appendSlice(_.alloc, "]}");
        return try out.toOwnedSlice(_.alloc);
    }

    fn buildPromptsListResult(): ![]u8 {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(_.alloc);

        try out.appendSlice(_.alloc, "{\"prompts\":[");
        let idx: usize = 0;
        while (idx < _.prompts.items.len) {
            const p = _.prompts.items[idx];
            if (idx > 0) try out.append(_.alloc, ',');
            try out.appendSlice(_.alloc, "{\"name\":");
            try proto.appendJsonString(&out, _.alloc, p.def.name);
            try out.appendSlice(_.alloc, ",\"description\":");
            try proto.appendJsonString(&out, _.alloc, p.def.description);
            try out.appendSlice(_.alloc, ",\"arguments\":");
            try out.appendSlice(_.alloc, p.def.arguments_json);
            try out.append(_.alloc, '}');
            idx += 1;
        }
        try out.appendSlice(_.alloc, "]}");
        return try out.toOwnedSlice(_.alloc);
    }

    pub fn handleMessage(*_, raw_json: []const u8): !([]u8)? {
        let req = proto.decodeRequest(_.alloc, raw_json) catch |e| {
            if (e == error.ParseError) {
                return try proto.encodeErrorResponse(_.alloc, proto.McpId.none(), proto.JSONRPC_PARSE_ERROR, "Parse error", null);
            }
            return try proto.encodeErrorResponse(_.alloc, proto.McpId.none(), proto.JSONRPC_INVALID_REQUEST, "Invalid Request", null);
        };
        defer req.deinit(_.alloc);

        if (std.mem.eql(u8, req.method, "initialize")) {
            _.initialized = true;
            if (req.is_notification) {
                return null;
            }
            const res_json = try _.buildInitializeResult();
            defer _.alloc.free(res_json);
            return try proto.encodeResponse(_.alloc, proto.McpResponse{
                .id = req.id,
                .result_json = res_json,
            });
        }

        if (std.mem.eql(u8, req.method, "notifications/initialized")) {
            _.initialized = true;
            return null;
        }

        if (std.mem.eql(u8, req.method, "ping")) {
            if (req.is_notification) {
                return null;
            }
            return try proto.encodeResponse(_.alloc, proto.McpResponse{
                .id = req.id,
                .result_json = "{}",
            });
        }

        if (!_.initialized) {
            if (req.is_notification) {
                return null;
            }
            return try proto.encodeErrorResponse(_.alloc, req.id, proto.JSONRPC_INVALID_REQUEST, "Server not initialized", null);
        }

        if (std.mem.eql(u8, req.method, "tools/list")) {
            const res_json = try _.buildToolsListResult();
            defer _.alloc.free(res_json);
            return try proto.encodeResponse(_.alloc, proto.McpResponse{
                .id = req.id,
                .result_json = res_json,
            });
        }

        if (std.mem.eql(u8, req.method, "tools/call")) {
            const tool_name_opt = try proto.extractJsonStringField(_.alloc, req.params_json, "name");
            if (tool_name_opt == null) {
                return try proto.encodeErrorResponse(_.alloc, req.id, proto.JSONRPC_INVALID_PARAMS, "Missing tool name", null);
            }
            const tool_name = tool_name_opt.?;
            defer _.alloc.free(tool_name);

            const args_json = proto.extractJsonRawField(req.params_json, "arguments") orelse "{}";
            for (t of _.tools.items) {
                if (std.mem.eql(u8, t.def.name, tool_name)) {
                    const tool_res = t.handler(_.alloc, args_json) catch |e| {
                        const err_msg = try std.fmt.allocPrint(_.alloc, "Tool execution failed: {s}", .{@errorName(e)});
                        defer _.alloc.free(err_msg);
                        const err_items = [proto.McpContent.makeText(err_msg)];
                        const err_res = proto.McpToolResult.fail(&err_items);
                        const res_json = try _.encodeToolResult(err_res);
                        defer _.alloc.free(res_json);
                        return try proto.encodeResponse(_.alloc, proto.McpResponse{
                            .id = req.id,
                            .result_json = res_json,
                        });
                    };
                    const res_json = try _.encodeToolResult(tool_res);
                    defer _.alloc.free(res_json);
                    for (c of tool_res.content) {
                        _.alloc.free(c.text);
                    }
                    if (tool_res.content.len > 0) {
                        _.alloc.free(tool_res.content);
                    }
                    return try proto.encodeResponse(_.alloc, proto.McpResponse{
                        .id = req.id,
                        .result_json = res_json,
                    });
                }
            }
            return try proto.encodeErrorResponse(_.alloc, req.id, proto.JSONRPC_INVALID_PARAMS, "Unknown tool", tool_name);
        }

        if (std.mem.eql(u8, req.method, "resources/list")) {
            const res_json = try _.buildResourcesListResult();
            defer _.alloc.free(res_json);
            return try proto.encodeResponse(_.alloc, proto.McpResponse{
                .id = req.id,
                .result_json = res_json,
            });
        }

        if (std.mem.eql(u8, req.method, "resources/read")) {
            const uri_opt = try proto.extractJsonStringField(_.alloc, req.params_json, "uri");
            if (uri_opt == null) {
                return try proto.encodeErrorResponse(_.alloc, req.id, proto.JSONRPC_INVALID_PARAMS, "Missing resource uri", null);
            }
            const uri = uri_opt.?;
            defer _.alloc.free(uri);

            for (r of _.resources.items) {
                if (std.mem.eql(u8, r.def.uri, uri)) {
                    const body = try r.reader(_.alloc, uri);
                    defer _.alloc.free(body);

                    let out = std.ArrayList(u8).empty;
                    errdefer out.deinit(_.alloc);
                    try out.appendSlice(_.alloc, "{\"contents\":[{\"uri\":");
                    try proto.appendJsonString(&out, _.alloc, uri);
                    try out.appendSlice(_.alloc, ",\"mimeType\":");
                    try proto.appendJsonString(&out, _.alloc, r.def.mime_type);
                    try out.appendSlice(_.alloc, ",\"text\":");
                    try proto.appendJsonString(&out, _.alloc, body);
                    try out.appendSlice(_.alloc, "}]}");
                    const res_json = try out.toOwnedSlice(_.alloc);
                    defer _.alloc.free(res_json);
                    return try proto.encodeResponse(_.alloc, proto.McpResponse{
                        .id = req.id,
                        .result_json = res_json,
                    });
                }
            }
            return try proto.encodeErrorResponse(_.alloc, req.id, proto.JSONRPC_INVALID_PARAMS, "Unknown resource", uri);
        }

        if (std.mem.eql(u8, req.method, "prompts/list")) {
            const res_json = try _.buildPromptsListResult();
            defer _.alloc.free(res_json);
            return try proto.encodeResponse(_.alloc, proto.McpResponse{
                .id = req.id,
                .result_json = res_json,
            });
        }

        if (std.mem.eql(u8, req.method, "prompts/get")) {
            const prompt_name_opt = try proto.extractJsonStringField(_.alloc, req.params_json, "name");
            if (prompt_name_opt == null) {
                return try proto.encodeErrorResponse(_.alloc, req.id, proto.JSONRPC_INVALID_PARAMS, "Missing prompt name", null);
            }
            const prompt_name = prompt_name_opt.?;
            defer _.alloc.free(prompt_name);

            const args_json = proto.extractJsonRawField(req.params_json, "arguments") orelse "{}";
            for (p of _.prompts.items) {
                if (std.mem.eql(u8, p.def.name, prompt_name)) {
                    const prompt_text = try p.builder(_.alloc, args_json);
                    defer _.alloc.free(prompt_text);

                    let out = std.ArrayList(u8).empty;
                    errdefer out.deinit(_.alloc);
                    try out.appendSlice(_.alloc, "{\"description\":");
                    try proto.appendJsonString(&out, _.alloc, p.def.description);
                    try out.appendSlice(_.alloc, ",\"messages\":[{\"role\":\"user\",\"content\":{\"type\":\"text\",\"text\":");
                    try proto.appendJsonString(&out, _.alloc, prompt_text);
                    try out.appendSlice(_.alloc, "}}]}");
                    const res_json = try out.toOwnedSlice(_.alloc);
                    defer _.alloc.free(res_json);
                    return try proto.encodeResponse(_.alloc, proto.McpResponse{
                        .id = req.id,
                        .result_json = res_json,
                    });
                }
            }
            return try proto.encodeErrorResponse(_.alloc, req.id, proto.JSONRPC_INVALID_PARAMS, "Unknown prompt", prompt_name);
        }

        if (req.is_notification) {
            return null;
        }
        return try proto.encodeErrorResponse(_.alloc, req.id, proto.JSONRPC_METHOD_NOT_FOUND, "Method not found", req.method);
    }
}
