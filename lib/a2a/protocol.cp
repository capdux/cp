// Source: cp/lib/a2a/protocol.cp (A2A protocol types, AgentCard, TaskState, Part, Message, Artifact, JSON & SSE codecs)
const std = @import("std");

pub const A2A_PROTOCOL_VERSION: []const u8 = "0.2.1";

pub const JSONRPC_PARSE_ERROR: i32 = -32700;
pub const JSONRPC_INVALID_REQUEST: i32 = -32600;
pub const JSONRPC_METHOD_NOT_FOUND: i32 = -32601;
pub const JSONRPC_INVALID_PARAMS: i32 = -32602;
pub const JSONRPC_INTERNAL_ERROR: i32 = -32603;

pub const A2A_ERR_TASK_NOT_FOUND: i32 = -32001;
pub const A2A_ERR_TASK_NOT_CANCELABLE: i32 = -32002;
pub const A2A_ERR_PUSH_NOT_SUPPORTED: i32 = -32003;
pub const A2A_ERR_UNSUPPORTED_OPERATION: i32 = -32004;
pub const A2A_ERR_INVALID_STATE_TRANSITION: i32 = -32005;

pub const A2aError = error{
    InvalidJson,
    InvalidRequest,
    MethodNotFound,
    InvalidParams,
    TaskNotFound,
    TaskNotCancelable,
    InvalidStateTransition,
    CapacityExceeded,
    InvalidSseEvent,
    OutOfMemory,
};

pub struct AgentSkill {
    id: []const u8,
    name: []const u8,
    description: []const u8 = "",
    tags: []const []const u8 = &.{},
}

pub struct AgentCapabilities {
    streaming: bool = false,
    push_notifications: bool = false,
    state_transition_history: bool = true,
}

pub struct AgentCard {
    name: []const u8,
    description: []const u8 = "",
    url: []const u8,
    version: []const u8 = "1.0.0",
    protocol_version: []const u8 = A2A_PROTOCOL_VERSION,
    capabilities: AgentCapabilities = .{},
    default_input_modes: []const []const u8 = &.{"text/plain"},
    default_output_modes: []const []const u8 = &.{"text/plain"},
    skills: []const AgentSkill = &.{},
}

pub struct AgentCardSummary {
    name: []u8,
    url: []u8,
    version: []u8,
    streaming: bool,
    push_notifications: bool,
    skill_count: usize,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.name);
        alloc.free(_.url);
        alloc.free(_.version);
    }
}

pub enum TaskState {
    Submitted,
    Working,
    InputRequired,
    Completed,
    Failed,
    Canceled,
    Rejected,

    pub fn toString(): []const u8 {
        return switch (_) {
            .Submitted => "submitted",
            .Working => "working",
            .InputRequired => "input-required",
            .Completed => "completed",
            .Failed => "failed",
            .Canceled => "canceled",
            .Rejected => "rejected",
        };
    }

    pub static fn fromString(s: []const u8): TaskState? {
        if (std.mem.eql(u8, s, "submitted")) {
            return .Submitted;
        }
        if (std.mem.eql(u8, s, "working")) {
            return .Working;
        }
        if (std.mem.eql(u8, s, "input-required")) {
            return .InputRequired;
        }
        if (std.mem.eql(u8, s, "completed")) {
            return .Completed;
        }
        if (std.mem.eql(u8, s, "failed")) {
            return .Failed;
        }
        if (std.mem.eql(u8, s, "canceled")) {
            return .Canceled;
        }
        if (std.mem.eql(u8, s, "rejected")) {
            return .Rejected;
        }
        return null;
    }

    pub fn isTerminal(): bool {
        return switch (_) {
            .Completed, .Failed, .Canceled, .Rejected => true,
            .Submitted, .Working, .InputRequired => false,
        };
    }

    pub fn canTransitionTo(next: TaskState): bool {
        if (_.isTerminal()) {
            return false;
        }
        return switch (next) {
            .Submitted => false,
            .Working, .InputRequired, .Completed, .Failed, .Canceled, .Rejected => true,
        };
    }
}

pub enum MessageRole {
    User,
    Agent,

    pub fn toString(): []const u8 {
        return switch (_) {
            .User => "user",
            .Agent => "agent",
        };
    }

    pub static fn fromString(s: []const u8): MessageRole? {
        if (std.mem.eql(u8, s, "user")) {
            return .User;
        }
        if (std.mem.eql(u8, s, "agent")) {
            return .Agent;
        }
        return null;
    }
}

pub enum PartKind {
    Text,
    DataJson,
    FileUri,

    pub fn toString(): []const u8 {
        return switch (_) {
            .Text => "text",
            .DataJson => "data",
            .FileUri => "file",
        };
    }

    pub static fn fromString(s: []const u8): PartKind? {
        if (std.mem.eql(u8, s, "text")) {
            return .Text;
        }
        if (std.mem.eql(u8, s, "data")) {
            return .DataJson;
        }
        if (std.mem.eql(u8, s, "file")) {
            return .FileUri;
        }
        return null;
    }
}

pub struct Part {
    kind: PartKind,
    text: []const u8 = "",
    mime_type: []const u8 = "text/plain",
    uri: []const u8 = "",
    data_json: []const u8 = "",

    pub static fn textPart(t: []const u8): Part {
        return .{ .kind = .Text, .text = t };
    }

    pub static fn dataPart(json_payload: []const u8): Part {
        return .{ .kind = .DataJson, .mime_type = "application/json", .data_json = json_payload };
    }

    pub static fn filePart(file_uri: []const u8, mime: []const u8): Part {
        return .{ .kind = .FileUri, .uri = file_uri, .mime_type = mime };
    }
}

pub struct Message {
    role: MessageRole,
    parts: []const Part,
    message_id: []const u8 = "",
}

pub struct Artifact {
    artifact_id: []const u8,
    name: []const u8 = "",
    description: []const u8 = "",
    parts: []const Part,
    index: usize = 0,
    append: bool = false,
    last_chunk: bool = true,
}

pub enum SseEventKind {
    StatusUpdate,
    ArtifactUpdate,
}

pub struct TaskStatusUpdateEvent {
    task_id: []const u8,
    context_id: []const u8 = "",
    state: TaskState,
    message: Message? = null,
    final: bool = false,
}

pub struct TaskArtifactUpdateEvent {
    task_id: []const u8,
    context_id: []const u8 = "",
    artifact: Artifact,
}

pub struct SseTaskEvent {
    kind: SseEventKind,
    status: TaskStatusUpdateEvent? = null,
    artifact: TaskArtifactUpdateEvent? = null,
}

pub struct ParsedSseTaskEvent {
    kind: SseEventKind,
    task_id: []u8,
    context_id: []u8,
    state: TaskState? = null,
    final: bool = false,
    message_text: ([]u8)? = null,
    artifact_id: ([]u8)? = null,
    artifact_name: ([]u8)? = null,
    artifact_text: ([]u8)? = null,
    index: usize = 0,
    append: bool = false,
    last_chunk: bool = true,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.task_id);
        alloc.free(_.context_id);
        if (t := _.message_text) {
            alloc.free(t);
        }
        if (aid := _.artifact_id) {
            alloc.free(aid);
        }
        if (aname := _.artifact_name) {
            alloc.free(aname);
        }
        if (atext := _.artifact_text) {
            alloc.free(atext);
        }
    }
}

pub fn appendJsonString(out: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): !void {
    try out.append(alloc, '"');
    for (c of s) {
        if (c == '"') {
            try out.appendSlice(alloc, "\\\"");
        } else if (c == '\\') {
            try out.appendSlice(alloc, "\\\\");
        } else if (c == '\x08') {
            try out.appendSlice(alloc, "\\b");
        } else if (c == '\x0c') {
            try out.appendSlice(alloc, "\\f");
        } else if (c == '\n') {
            try out.appendSlice(alloc, "\\n");
        } else if (c == '\r') {
            try out.appendSlice(alloc, "\\r");
        } else if (c == '\t') {
            try out.appendSlice(alloc, "\\t");
        } else if (c < 0x20) {
            const hex = "0123456789abcdef";
            try out.appendSlice(alloc, "\\u00");
            try out.append(alloc, hex[(c >> 4) & 0x0f]);
            try out.append(alloc, hex[c & 0x0f]);
        } else {
            try out.append(alloc, c);
        }
    }
    try out.append(alloc, '"');
}

fn findJsonFieldValueStart(json: []const u8, key: []const u8): usize? {
    let i: usize = 0;
    let depth: usize = 0;
    let in_string: bool = false;
    let escaped: bool = false;

    while (i < json.len) {
        const c = json[i];
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                in_string = false;
            }
            i += 1;
            continue;
        }

        if (c == '{' or c == '[') {
            depth += 1;
            i += 1;
            continue;
        }
        if (c == '}' or c == ']') {
            if (depth > 0) {
                depth -= 1;
            }
            i += 1;
            continue;
        }

        if (c == '"') {
            const key_start = i + 1;
            let j = key_start;
            let key_esc = false;
            while (j < json.len) {
                const kc = json[j];
                if (key_esc) {
                    key_esc = false;
                } else if (kc == '\\') {
                    key_esc = true;
                } else if (kc == '"') {
                    break;
                }
                j += 1;
            }
            if (j >= json.len) {
                return null;
            }
            const found_key = json[key_start..j];
            let k = j + 1;
            while (k < json.len and (json[k] == ' ' or json[k] == '\t' or json[k] == '\n' or json[k] == '\r')) {
                k += 1;
            }
            if (k < json.len and json[k] == ':') {
                k += 1;
                while (k < json.len and (json[k] == ' ' or json[k] == '\t' or json[k] == '\n' or json[k] == '\r')) {
                    k += 1;
                }
                if (std.mem.eql(u8, found_key, key)) {
                    return k;
                }
            }
            i = j + 1;
            continue;
        }

        i += 1;
    }
    return null;
}

pub fn extractJsonRawField(json: []const u8, key: []const u8): ([]const u8)? {
    const start = findJsonFieldValueStart(json, key) orelse return null;
    if (start >= json.len) {
        return null;
    }
    const first = json[start];
    if (first == '"') {
        let i = start + 1;
        let escaped = false;
        while (i < json.len) {
            const c = json[i];
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                return json[start .. i + 1];
            }
            i += 1;
        }
        return null;
    }
    if (first == '{' or first == '[') {
        const open = first;
        const close: u8 = if (open == '{') '}' else ']';
        let i = start;
        let depth: usize = 0;
        let in_str = false;
        let esc = false;
        while (i < json.len) {
            const c = json[i];
            if (in_str) {
                if (esc) {
                    esc = false;
                } else if (c == '\\') {
                    esc = true;
                } else if (c == '"') {
                    in_str = false;
                }
            } else if (c == '"') {
                in_str = true;
            } else if (c == open) {
                depth += 1;
            } else if (c == close) {
                depth -= 1;
                if (depth == 0) {
                    return json[start .. i + 1];
                }
            }
            i += 1;
        }
        return null;
    }
    let i = start;
    while (i < json.len) {
        const c = json[i];
        if (c == ',' or c == '}' or c == ']' or c == ' ' or c == '\t' or c == '\n' or c == '\r') {
            break;
        }
        i += 1;
    }
    return json[start..i];
}

pub fn extractJsonStringField(alloc: std.mem.Allocator, json: []const u8, key: []const u8): !([]u8)? {
    const raw = extractJsonRawField(json, key) orelse return null;
    if (raw.len < 2 or raw[0] != '"' or raw[raw.len - 1] != '"') {
        return null;
    }
    const inner = raw[1 .. raw.len - 1];
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    let i: usize = 0;
    while (i < inner.len) {
        if (inner[i] == '\\' and i + 1 < inner.len) {
            const next = inner[i + 1];
            if (next == '"') {
                try out.append(alloc, '"');
                i += 2;
            } else if (next == '\\') {
                try out.append(alloc, '\\');
                i += 2;
            } else if (next == '/') {
                try out.append(alloc, '/');
                i += 2;
            } else if (next == 'b') {
                try out.append(alloc, '\x08');
                i += 2;
            } else if (next == 'f') {
                try out.append(alloc, '\x0c');
                i += 2;
            } else if (next == 'n') {
                try out.append(alloc, '\n');
                i += 2;
            } else if (next == 'r') {
                try out.append(alloc, '\r');
                i += 2;
            } else if (next == 't') {
                try out.append(alloc, '\t');
                i += 2;
            } else if (next == 'u' and i + 5 < inner.len) {
                const hex_slice = inner[i + 2 .. i + 6];
                const unicode_cp = std.fmt.parseInt(u21, hex_slice, 16) catch {
                    try out.append(alloc, inner[i]);
                    i += 1;
                    continue;
                };
                let utf8_buf: [4]u8 = undefined;
                const n = std.unicode.utf8Encode(unicode_cp, &utf8_buf) catch 0;
                if (n > 0) {
                    try out.appendSlice(alloc, utf8_buf[0..n]);
                }
                i += 6;
            } else {
                try out.append(alloc, next);
                i += 2;
            }
        } else {
            try out.append(alloc, inner[i]);
            i += 1;
        }
    }
    return try out.toOwnedSlice(alloc);
}

pub fn encodePartJson(out: *std.ArrayList(u8), alloc: std.mem.Allocator, part: Part): !void {
    try out.appendSlice(alloc, "{\"kind\":");
    try appendJsonString(out, alloc, part.kind.toString());
    switch (part.kind) {
        .Text => {
            try out.appendSlice(alloc, ",\"text\":");
            try appendJsonString(out, alloc, part.text);
        },
        .DataJson => {
            try out.appendSlice(alloc, ",\"data\":");
            if (part.data_json.len > 0) {
                try out.appendSlice(alloc, part.data_json);
            } else {
                try out.appendSlice(alloc, "{}");
            }
        },
        .FileUri => {
            try out.appendSlice(alloc, ",\"file\":{\"uri\":");
            try appendJsonString(out, alloc, part.uri);
            try out.appendSlice(alloc, ",\"mimeType\":");
            try appendJsonString(out, alloc, part.mime_type);
            try out.append(alloc, '}');
        },
    }
    try out.append(alloc, '}');
}

pub fn encodeMessageJson(out: *std.ArrayList(u8), alloc: std.mem.Allocator, msg: Message): !void {
    try out.appendSlice(alloc, "{\"role\":");
    try appendJsonString(out, alloc, msg.role.toString());
    if (msg.message_id.len > 0) {
        try out.appendSlice(alloc, ",\"messageId\":");
        try appendJsonString(out, alloc, msg.message_id);
    }
    try out.appendSlice(alloc, ",\"parts\":[");
    let idx: usize = 0;
    for (p of msg.parts) {
        if (idx > 0) {
            try out.append(alloc, ',');
        }
        try encodePartJson(out, alloc, p);
        idx += 1;
    }
    try out.appendSlice(alloc, "]}");
}

pub fn encodeArtifactJson(out: *std.ArrayList(u8), alloc: std.mem.Allocator, art: Artifact): !void {
    try out.appendSlice(alloc, "{\"artifactId\":");
    try appendJsonString(out, alloc, art.artifact_id);
    if (art.name.len > 0) {
        try out.appendSlice(alloc, ",\"name\":");
        try appendJsonString(out, alloc, art.name);
    }
    if (art.description.len > 0) {
        try out.appendSlice(alloc, ",\"description\":");
        try appendJsonString(out, alloc, art.description);
    }
    try out.appendSlice(alloc, ",\"parts\":[");
    let idx: usize = 0;
    for (p of art.parts) {
        if (idx > 0) {
            try out.append(alloc, ',');
        }
        try encodePartJson(out, alloc, p);
        idx += 1;
    }
    try out.appendSlice(alloc, "]");
    let num_buf: [32]u8 = undefined;
    const idx_str = std.fmt.bufPrint(&num_buf, "{d}", .{art.index}) catch "0";
    try out.appendSlice(alloc, ",\"index\":");
    try out.appendSlice(alloc, idx_str);
    try out.appendSlice(alloc, ",\"append\":");
    try out.appendSlice(alloc, if (art.append) "true" else "false");
    try out.appendSlice(alloc, ",\"lastChunk\":");
    try out.appendSlice(alloc, if (art.last_chunk) "true" else "false");
    try out.append(alloc, '}');
}

pub fn encodeAgentCardJson(alloc: std.mem.Allocator, card: AgentCard): ![]u8 {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    try out.appendSlice(alloc, "{\"name\":");
    try appendJsonString(&out, alloc, card.name);
    try out.appendSlice(alloc, ",\"description\":");
    try appendJsonString(&out, alloc, card.description);
    try out.appendSlice(alloc, ",\"url\":");
    try appendJsonString(&out, alloc, card.url);
    try out.appendSlice(alloc, ",\"version\":");
    try appendJsonString(&out, alloc, card.version);
    try out.appendSlice(alloc, ",\"protocolVersion\":");
    try appendJsonString(&out, alloc, card.protocol_version);

    try out.appendSlice(alloc, ",\"capabilities\":{\"streaming\":");
    try out.appendSlice(alloc, if (card.capabilities.streaming) "true" else "false");
    try out.appendSlice(alloc, ",\"pushNotifications\":");
    try out.appendSlice(alloc, if (card.capabilities.push_notifications) "true" else "false");
    try out.appendSlice(alloc, ",\"stateTransitionHistory\":");
    try out.appendSlice(alloc, if (card.capabilities.state_transition_history) "true" else "false");
    try out.append(alloc, '}');

    try out.appendSlice(alloc, ",\"defaultInputModes\":[");
    let in_idx: usize = 0;
    for (m of card.default_input_modes) {
        if (in_idx > 0) {
            try out.append(alloc, ',');
        }
        try appendJsonString(&out, alloc, m);
        in_idx += 1;
    }
    try out.appendSlice(alloc, "],\"defaultOutputModes\":[");
    let out_idx: usize = 0;
    for (m of card.default_output_modes) {
        if (out_idx > 0) {
            try out.append(alloc, ',');
        }
        try appendJsonString(&out, alloc, m);
        out_idx += 1;
    }
    try out.appendSlice(alloc, "],\"skills\":[");
    let sidx: usize = 0;
    for (sk of card.skills) {
        if (sidx > 0) {
            try out.append(alloc, ',');
        }
        try out.appendSlice(alloc, "{\"id\":");
        try appendJsonString(&out, alloc, sk.id);
        try out.appendSlice(alloc, ",\"name\":");
        try appendJsonString(&out, alloc, sk.name);
        try out.appendSlice(alloc, ",\"description\":");
        try appendJsonString(&out, alloc, sk.description);
        try out.appendSlice(alloc, ",\"tags\":[");
        let tidx: usize = 0;
        for (tg of sk.tags) {
            if (tidx > 0) {
                try out.append(alloc, ',');
            }
            try appendJsonString(&out, alloc, tg);
            tidx += 1;
        }
        try out.appendSlice(alloc, "]}");
        sidx += 1;
    }
    try out.appendSlice(alloc, "]}");
    return try out.toOwnedSlice(alloc);
}

fn countTopLevelArrayItems(arr_json: []const u8): usize {
    if (arr_json.len < 2 or arr_json[0] != '[') {
        return 0;
    }
    let i: usize = 1;
    let depth: usize = 0;
    let in_str = false;
    let esc = false;
    let count: usize = 0;
    let has_token = false;

    while (i + 1 < arr_json.len) {
        const c = arr_json[i];
        if (in_str) {
            if (esc) {
                esc = false;
            } else if (c == '\\') {
                esc = true;
            } else if (c == '"') {
                in_str = false;
            }
            i += 1;
            continue;
        }
        if (c == '"') {
            in_str = true;
            has_token = true;
        } else if (c == '{' or c == '[') {
            depth += 1;
            has_token = true;
        } else if (c == '}' or c == ']') {
            if (depth > 0) {
                depth -= 1;
            }
        } else if (c == ',' and depth == 0) {
            if (has_token) {
                count += 1;
                has_token = false;
            }
        } else if (c != ' ' and c != '\t' and c != '\n' and c != '\r') {
            has_token = true;
        }
        i += 1;
    }
    if (has_token) {
        count += 1;
    }
    return count;
}

pub fn parseAgentCardSummary(alloc: std.mem.Allocator, json: []const u8): !AgentCardSummary {
    const name = (try extractJsonStringField(alloc, json, "name")) orelse return A2aError.InvalidJson;
    errdefer alloc.free(name);
    const url = (try extractJsonStringField(alloc, json, "url")) orelse return A2aError.InvalidJson;
    errdefer alloc.free(url);
    const version = (try extractJsonStringField(alloc, json, "version")) orelse try alloc.dupe(u8, "1.0.0");
    errdefer alloc.free(version);

    let streaming = false;
    let push_notifications = false;
    if (caps_raw := extractJsonRawField(json, "capabilities")) {
        if (st := extractJsonRawField(caps_raw, "streaming")) {
            streaming = std.mem.eql(u8, st, "true");
        }
        if (pn := extractJsonRawField(caps_raw, "pushNotifications")) {
            push_notifications = std.mem.eql(u8, pn, "true");
        }
    }

    let skill_count: usize = 0;
    if (skills_raw := extractJsonRawField(json, "skills")) {
        skill_count = countTopLevelArrayItems(skills_raw);
    }

    return .{
        .name = name,
        .url = url,
        .version = version,
        .streaming = streaming,
        .push_notifications = push_notifications,
        .skill_count = skill_count,
    };
}

pub fn encodeTaskEventSse(alloc: std.mem.Allocator, event: SseTaskEvent): ![]u8 {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    switch (event.kind) {
        .StatusUpdate => {
            const st = event.status orelse return A2aError.InvalidParams;
            try out.appendSlice(alloc, "event: task_status_update\ndata: {\"kind\":\"status-update\",\"taskId\":");
            try appendJsonString(&out, alloc, st.task_id);
            if (st.context_id.len > 0) {
                try out.appendSlice(alloc, ",\"contextId\":");
                try appendJsonString(&out, alloc, st.context_id);
            }
            try out.appendSlice(alloc, ",\"status\":{\"state\":");
            try appendJsonString(&out, alloc, st.state.toString());
            if (msg := st.message) {
                try out.appendSlice(alloc, ",\"message\":");
                try encodeMessageJson(&out, alloc, msg);
            }
            try out.appendSlice(alloc, "},\"final\":");
            try out.appendSlice(alloc, if (st.final) "true" else "false");
            try out.appendSlice(alloc, "}\n\n");
        },
        .ArtifactUpdate => {
            const art_ev = event.artifact orelse return A2aError.InvalidParams;
            try out.appendSlice(alloc, "event: task_artifact_update\ndata: {\"kind\":\"artifact-update\",\"taskId\":");
            try appendJsonString(&out, alloc, art_ev.task_id);
            if (art_ev.context_id.len > 0) {
                try out.appendSlice(alloc, ",\"contextId\":");
                try appendJsonString(&out, alloc, art_ev.context_id);
            }
            try out.appendSlice(alloc, ",\"artifact\":");
            try encodeArtifactJson(&out, alloc, art_ev.artifact);
            try out.appendSlice(alloc, "}\n\n");
        },
    }

    return try out.toOwnedSlice(alloc);
}

pub fn parseSseTaskEvent(alloc: std.mem.Allocator, raw_sse: []const u8): !ParsedSseTaskEvent {
    let event_name: []const u8 = "";
    let data_json: []const u8 = "";

    let it = std.mem.splitScalar(u8, raw_sse, '\n');
    while (line_raw := it.next()) {
        const line = std.mem.trimRight(u8, line_raw, "\r");
        if (std.mem.startsWith(u8, line, "event:")) {
            event_name = std.mem.trim(u8, line[6..], " \t");
        } else if (std.mem.startsWith(u8, line, "data:")) {
            data_json = std.mem.trim(u8, line[5..], " \t");
        }
    }

    if (data_json.len == 0) {
        return A2aError.InvalidSseEvent;
    }

    let kind: SseEventKind = undefined;
    if (std.mem.eql(u8, event_name, "task_status_update")) {
        kind = .StatusUpdate;
    } else if (std.mem.eql(u8, event_name, "task_artifact_update")) {
        kind = .ArtifactUpdate;
    } else {
        const k_str = (try extractJsonStringField(alloc, data_json, "kind")) orelse return A2aError.InvalidSseEvent;
        defer alloc.free(k_str);
        if (std.mem.eql(u8, k_str, "status-update")) {
            kind = .StatusUpdate;
        } else if (std.mem.eql(u8, k_str, "artifact-update")) {
            kind = .ArtifactUpdate;
        } else {
            return A2aError.InvalidSseEvent;
        }
    }

    const task_id = (try extractJsonStringField(alloc, data_json, "taskId")) orelse return A2aError.InvalidSseEvent;
    errdefer alloc.free(task_id);
    const context_id = (try extractJsonStringField(alloc, data_json, "contextId")) orelse try alloc.dupe(u8, "");
    errdefer alloc.free(context_id);

    let res = ParsedSseTaskEvent{
        .kind = kind,
        .task_id = task_id,
        .context_id = context_id,
    };
    errdefer res.deinit(alloc);

    if (kind == .StatusUpdate) {
        if (status_raw := extractJsonRawField(data_json, "status")) {
            if (st_str := try extractJsonStringField(alloc, status_raw, "state")) {
                defer alloc.free(st_str);
                res.state = TaskState.fromString(st_str) orelse return A2aError.InvalidSseEvent;
            }
            if (msg_raw := extractJsonRawField(status_raw, "message")) {
                res.message_text = try extractJsonStringField(alloc, msg_raw, "text");
            }
        }
        if (fin_raw := extractJsonRawField(data_json, "final")) {
            res.final = std.mem.eql(u8, fin_raw, "true");
        }
    } else {
        const art_raw = extractJsonRawField(data_json, "artifact") orelse return A2aError.InvalidSseEvent;
        res.artifact_id = try extractJsonStringField(alloc, art_raw, "artifactId");
        res.artifact_name = try extractJsonStringField(alloc, art_raw, "name");
        res.artifact_text = try extractJsonStringField(alloc, art_raw, "text");
        if (idx_raw := extractJsonRawField(art_raw, "index")) {
            res.index = std.fmt.parseInt(usize, idx_raw, 10) catch 0;
        }
        if (app_raw := extractJsonRawField(art_raw, "append")) {
            res.append = std.mem.eql(u8, app_raw, "true");
        }
        if (lc_raw := extractJsonRawField(art_raw, "lastChunk")) {
            res.last_chunk = std.mem.eql(u8, lc_raw, "true");
        }
    }

    return res;
}
