// Source: cp/lib/a2a/server.cp (A2A JSON-RPC task router, lifecycle state machine, capacity guards)
const std = @import("std");
const proto = @import("./protocol.cp");

pub struct A2aServerConfig {
    max_tasks: usize = 1024,
    max_history_per_task: usize = 128,
    max_artifacts_per_task: usize = 64,
}

struct OwnedPart {
    kind: proto.PartKind,
    text: []u8,
    mime_type: []u8,
    uri: []u8,
    data_json: []u8,

    fn fromPart(alloc: std.mem.Allocator, p: proto.Part): !OwnedPart {
        const t = try alloc.dupe(u8, p.text);
        errdefer alloc.free(t);
        const m = try alloc.dupe(u8, p.mime_type);
        errdefer alloc.free(m);
        const u = try alloc.dupe(u8, p.uri);
        errdefer alloc.free(u);
        const d = try alloc.dupe(u8, p.data_json);
        return .{
            .kind = p.kind,
            .text = t,
            .mime_type = m,
            .uri = u,
            .data_json = d,
        };
    }

    fn toPart(): proto.Part {
        return .{
            .kind = _.kind,
            .text = _.text,
            .mime_type = _.mime_type,
            .uri = _.uri,
            .data_json = _.data_json,
        };
    }

    fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.text);
        alloc.free(_.mime_type);
        alloc.free(_.uri);
        alloc.free(_.data_json);
    }
}

struct OwnedMessage {
    role: proto.MessageRole,
    message_id: []u8,
    parts: std.ArrayList(OwnedPart),

    fn fromMessage(alloc: std.mem.Allocator, msg: proto.Message): !OwnedMessage {
        const mid = try alloc.dupe(u8, msg.message_id);
        errdefer alloc.free(mid);
        let parts = std.ArrayList(OwnedPart).empty;
        errdefer {
            for (*op of parts.items) {
                op.deinit(alloc);
            }
            parts.deinit(alloc);
        }
        for (p of msg.parts) {
            const op = try OwnedPart.fromPart(alloc, p);
            try parts.append(alloc, op);
        }
        return .{
            .role = msg.role,
            .message_id = mid,
            .parts = parts,
        };
    }

    fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.message_id);
        for (*op of _.parts.items) {
            op.deinit(alloc);
        }
        _.parts.deinit(alloc);
    }
}

struct OwnedArtifact {
    artifact_id: []u8,
    name: []u8,
    description: []u8,
    parts: std.ArrayList(OwnedPart),
    index: usize,
    append: bool,
    last_chunk: bool,

    fn fromArtifact(alloc: std.mem.Allocator, art: proto.Artifact): !OwnedArtifact {
        const aid = try alloc.dupe(u8, art.artifact_id);
        errdefer alloc.free(aid);
        const nm = try alloc.dupe(u8, art.name);
        errdefer alloc.free(nm);
        const desc = try alloc.dupe(u8, art.description);
        errdefer alloc.free(desc);
        let parts = std.ArrayList(OwnedPart).empty;
        errdefer {
            for (*op of parts.items) {
                op.deinit(alloc);
            }
            parts.deinit(alloc);
        }
        for (p of art.parts) {
            const op = try OwnedPart.fromPart(alloc, p);
            try parts.append(alloc, op);
        }
        return .{
            .artifact_id = aid,
            .name = nm,
            .description = desc,
            .parts = parts,
            .index = art.index,
            .append = art.append,
            .last_chunk = art.last_chunk,
        };
    }

    fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.artifact_id);
        alloc.free(_.name);
        alloc.free(_.description);
        for (*op of _.parts.items) {
            op.deinit(alloc);
        }
        _.parts.deinit(alloc);
    }
}

pub struct A2aTask {
    id: []u8,
    context_id: []u8,
    state: proto.TaskState,
    history: std.ArrayList(OwnedMessage),
    artifacts: std.ArrayList(OwnedArtifact),

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.id);
        alloc.free(_.context_id);
        for (*m of _.history.items) {
            m.deinit(alloc);
        }
        _.history.deinit(alloc);
        for (*a of _.artifacts.items) {
            a.deinit(alloc);
        }
        _.artifacts.deinit(alloc);
    }

    pub fn historyLen(): usize {
        return _.history.items.len;
    }

    pub fn artifactLen(): usize {
        return _.artifacts.items.len;
    }
}

pub struct A2aServer {
    alloc: std.mem.Allocator,
    config: A2aServerConfig,
    card: proto.AgentCard,
    tasks: std.ArrayList(A2aTask),

    pub fn init(alloc: std.mem.Allocator, card: proto.AgentCard, config: A2aServerConfig): A2aServer {
        return .{
            .alloc = alloc,
            .config = config,
            .card = card,
            .tasks = std.ArrayList(A2aTask).empty,
        };
    }

    pub fn deinit(*_) {
        for (*t of _.tasks.items) {
            t.deinit(_.alloc);
        }
        _.tasks.deinit(_.alloc);
    }

    pub fn taskCount(): usize {
        return _.tasks.items.len;
    }

    fn findTaskIndex(task_id: []const u8): usize? {
        let idx: usize = 0;
        for (t of _.tasks.items) {
            if (std.mem.eql(u8, t.id, task_id)) {
                return idx;
            }
            idx += 1;
        }
        return null;
    }

    pub fn getTask(task_id: []const u8): (*const A2aTask)? {
        const idx = _.findTaskIndex(task_id) orelse return null;
        return &_.tasks.items[idx];
    }

    pub fn sendTask(
        *_,
        task_id: []const u8,
        context_id: []const u8,
        msg: proto.Message,
    ): !proto.TaskState {
        if (task_id.len == 0) {
            return proto.A2aError.InvalidParams;
        }
        if (idx := _.findTaskIndex(task_id)) {
            let task = &_.tasks.items[idx];
            if (!task.state.canTransitionTo(.Working)) {
                return proto.A2aError.InvalidStateTransition;
            }
            if (task.history.items.len >= _.config.max_history_per_task) {
                return proto.A2aError.CapacityExceeded;
            }
            let om = try OwnedMessage.fromMessage(_.alloc, msg);
            errdefer om.deinit(_.alloc);
            try task.history.append(_.alloc, om);
            task.state = .Working;
            return task.state;
        }

        if (_.tasks.items.len >= _.config.max_tasks) {
            return proto.A2aError.CapacityExceeded;
        }
        const id_copy = try _.alloc.dupe(u8, task_id);
        errdefer _.alloc.free(id_copy);
        const ctx_copy = try _.alloc.dupe(u8, context_id);
        errdefer _.alloc.free(ctx_copy);

        let task = A2aTask{
            .id = id_copy,
            .context_id = ctx_copy,
            .state = .Working,
            .history = std.ArrayList(OwnedMessage).empty,
            .artifacts = std.ArrayList(OwnedArtifact).empty,
        };
        errdefer task.deinit(_.alloc);

        if (_.config.max_history_per_task == 0) {
            return proto.A2aError.CapacityExceeded;
        }
        const om = try OwnedMessage.fromMessage(_.alloc, msg);
        try task.history.append(_.alloc, om);
        try _.tasks.append(_.alloc, task);
        return .Working;
    }

    pub fn transitionTask(
        *_,
        task_id: []const u8,
        next_state: proto.TaskState,
        agent_msg: proto.Message?,
    ): !void {
        const idx = _.findTaskIndex(task_id) orelse return proto.A2aError.TaskNotFound;
        let task = &_.tasks.items[idx];
        if (!task.state.canTransitionTo(next_state)) {
            return proto.A2aError.InvalidStateTransition;
        }
        if (msg := agent_msg) {
            if (task.history.items.len >= _.config.max_history_per_task) {
                return proto.A2aError.CapacityExceeded;
            }
            const om = try OwnedMessage.fromMessage(_.alloc, msg);
            try task.history.append(_.alloc, om);
        }
        task.state = next_state;
    }

    pub fn addArtifact(*_, task_id: []const u8, art: proto.Artifact): !void {
        const idx = _.findTaskIndex(task_id) orelse return proto.A2aError.TaskNotFound;
        let task = &_.tasks.items[idx];
        if (task.artifacts.items.len >= _.config.max_artifacts_per_task) {
            return proto.A2aError.CapacityExceeded;
        }
        const oa = try OwnedArtifact.fromArtifact(_.alloc, art);
        try task.artifacts.append(_.alloc, oa);
    }

    pub fn cancelTask(*_, task_id: []const u8): !proto.TaskState {
        const idx = _.findTaskIndex(task_id) orelse return proto.A2aError.TaskNotFound;
        let task = &_.tasks.items[idx];
        if (task.state.isTerminal()) {
            return proto.A2aError.TaskNotCancelable;
        }
        task.state = .Canceled;
        return .Canceled;
    }

    fn encodeRpcError(id_raw: []const u8, code: i32, msg: []const u8): ![]u8 {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(_.alloc);
        let code_buf: [24]u8 = undefined;
        const code_str = std.fmt.bufPrint(&code_buf, "{d}", .{code}) catch "-32603";
        try out.appendSlice(_.alloc, "{\"jsonrpc\":\"2.0\",\"id\":");
        try out.appendSlice(_.alloc, id_raw);
        try out.appendSlice(_.alloc, ",\"error\":{\"code\":");
        try out.appendSlice(_.alloc, code_str);
        try out.appendSlice(_.alloc, ",\"message\":");
        try proto.appendJsonString(&out, _.alloc, msg);
        try out.appendSlice(_.alloc, "}}");
        return try out.toOwnedSlice(_.alloc);
    }

    fn encodeTaskResult(id_raw: []const u8, task: *const A2aTask, history_limit: usize?): ![]u8 {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(_.alloc);

        try out.appendSlice(_.alloc, "{\"jsonrpc\":\"2.0\",\"id\":");
        try out.appendSlice(_.alloc, id_raw);
        try out.appendSlice(_.alloc, ",\"result\":{\"id\":");
        try proto.appendJsonString(&out, _.alloc, task.id);
        if (task.context_id.len > 0) {
            try out.appendSlice(_.alloc, ",\"contextId\":");
            try proto.appendJsonString(&out, _.alloc, task.context_id);
        }
        try out.appendSlice(_.alloc, ",\"status\":{\"state\":");
        try proto.appendJsonString(&out, _.alloc, task.state.toString());
        try out.appendSlice(_.alloc, "},\"history\":[");

        const total = task.history.items.len;
        let start_idx: usize = 0;
        if (lim := history_limit) {
            if (total > lim) {
                start_idx = total - lim;
            }
        }
        let i: usize = start_idx;
        while (i < total) {
            if (i > start_idx) {
                try out.append(_.alloc, ',');
            }
            const m = &task.history.items[i];
            try out.appendSlice(_.alloc, "{\"role\":");
            try proto.appendJsonString(&out, _.alloc, m.role.toString());
            try out.appendSlice(_.alloc, ",\"parts\":[");
            let pidx: usize = 0;
            for (op of m.parts.items) {
                if (pidx > 0) {
                    try out.append(_.alloc, ',');
                }
                try proto.encodePartJson(&out, _.alloc, op.toPart());
                pidx += 1;
            }
            try out.appendSlice(_.alloc, "]}");
            i += 1;
        }
        try out.appendSlice(_.alloc, "]}}");
        return try out.toOwnedSlice(_.alloc);
    }

    pub fn handleJsonRpc(*_, raw_req: []const u8): ![]u8 {
        const trimmed = std.mem.trim(u8, raw_req, " \t\n\r");
        if (trimmed.len < 2 or trimmed[0] != '{') {
            return try _.encodeRpcError("null", proto.JSONRPC_PARSE_ERROR, "Parse error");
        }
        const id_raw = proto.extractJsonRawField(trimmed, "id") orelse "null";
        const method = (try proto.extractJsonStringField(_.alloc, trimmed, "method")) orelse {
            return try _.encodeRpcError(id_raw, proto.JSONRPC_INVALID_REQUEST, "Missing method");
        };
        defer _.alloc.free(method);

        const params_raw = proto.extractJsonRawField(trimmed, "params") orelse trimmed;

        if (std.mem.eql(u8, method, "agent/card") or std.mem.eql(u8, method, "agent/authenticatedExtendedCard")) {
            const card_json = try proto.encodeAgentCardJson(_.alloc, _.card);
            defer _.alloc.free(card_json);
            let out = std.ArrayList(u8).empty;
            errdefer out.deinit(_.alloc);
            try out.appendSlice(_.alloc, "{\"jsonrpc\":\"2.0\",\"id\":");
            try out.appendSlice(_.alloc, id_raw);
            try out.appendSlice(_.alloc, ",\"result\":");
            try out.appendSlice(_.alloc, card_json);
            try out.append(_.alloc, '}');
            return try out.toOwnedSlice(_.alloc);
        }

        if (std.mem.eql(u8, method, "tasks/send")) {
            const task_id = (try proto.extractJsonStringField(_.alloc, params_raw, "id")) orelse {
                return try _.encodeRpcError(id_raw, proto.JSONRPC_INVALID_PARAMS, "Missing task id");
            };
            defer _.alloc.free(task_id);

            const ctx_id = (try proto.extractJsonStringField(_.alloc, params_raw, "contextId")) orelse try _.alloc.dupe(u8, "");
            defer _.alloc.free(ctx_id);

            const msg_raw = proto.extractJsonRawField(params_raw, "message") orelse params_raw;
            const text_opt = try proto.extractJsonStringField(_.alloc, msg_raw, "text");
            defer if (t := text_opt) _.alloc.free(t);
            const text = text_opt orelse "";

            let parts: [1]proto.Part = .{proto.Part.textPart(text)};
            const msg = proto.Message{
                .role = .User,
                .parts = &parts,
            };

            * = _.sendTask(task_id, ctx_id, msg) catch |e| {
                return switch (e) {
                    error.InvalidStateTransition => try _.encodeRpcError(id_raw, proto.A2A_ERR_INVALID_STATE_TRANSITION, "Task is in terminal state"),
                    error.CapacityExceeded => try _.encodeRpcError(id_raw, proto.JSONRPC_INTERNAL_ERROR, "Capacity exceeded"),
                    else => try _.encodeRpcError(id_raw, proto.JSONRPC_INVALID_PARAMS, "Invalid params"),
                };
            };
            const task = _.getTask(task_id) orelse return try _.encodeRpcError(id_raw, proto.JSONRPC_INTERNAL_ERROR, "Task lookup failed");
            return try _.encodeTaskResult(id_raw, task, null);
        }

        if (std.mem.eql(u8, method, "tasks/get")) {
            const task_id = (try proto.extractJsonStringField(_.alloc, params_raw, "id")) orelse {
                return try _.encodeRpcError(id_raw, proto.JSONRPC_INVALID_PARAMS, "Missing task id");
            };
            defer _.alloc.free(task_id);

            let history_limit: usize? = null;
            if (hl_raw := proto.extractJsonRawField(params_raw, "historyLength")) {
                history_limit = std.fmt.parseInt(usize, hl_raw, 10) catch null;
            }

            const task = _.getTask(task_id) orelse {
                return try _.encodeRpcError(id_raw, proto.A2A_ERR_TASK_NOT_FOUND, "Task not found");
            };
            return try _.encodeTaskResult(id_raw, task, history_limit);
        }

        if (std.mem.eql(u8, method, "tasks/cancel")) {
            const task_id = (try proto.extractJsonStringField(_.alloc, params_raw, "id")) orelse {
                return try _.encodeRpcError(id_raw, proto.JSONRPC_INVALID_PARAMS, "Missing task id");
            };
            defer _.alloc.free(task_id);

            * = _.cancelTask(task_id) catch |e| {
                return switch (e) {
                    error.TaskNotFound => try _.encodeRpcError(id_raw, proto.A2A_ERR_TASK_NOT_FOUND, "Task not found"),
                    error.TaskNotCancelable => try _.encodeRpcError(id_raw, proto.A2A_ERR_TASK_NOT_CANCELABLE, "Task is not cancelable"),
                    else => try _.encodeRpcError(id_raw, proto.JSONRPC_INTERNAL_ERROR, "Cancel failed"),
                };
            };
            const task = _.getTask(task_id) orelse return try _.encodeRpcError(id_raw, proto.JSONRPC_INTERNAL_ERROR, "Task lookup failed");
            return try _.encodeTaskResult(id_raw, task, null);
        }

        return try _.encodeRpcError(id_raw, proto.JSONRPC_METHOD_NOT_FOUND, "Method not found");
    }
}
