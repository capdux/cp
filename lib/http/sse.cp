// Source: cp/lib/http/sse.cp (WHATWG Server-Sent Events / AI Streaming Codec)
// lib/http/sse.cp
// Incremental Streaming WHATWG SSE Parser & Encoder for LLM / AI Agent Streaming (cap:http)

const std = @import("std");

pub const DEFAULT_MAX_SSE_EVENT_BYTES: usize = 1024 * 1024; // 1 MiB DoS protection limit

pub struct SseEvent {
    pub event_type: []const u8 = "message",
    pub data: []const u8 = "",
    pub id: []const u8 = "",
    pub retry_ms: u64? = null,
    priv owns_event_type: bool = false,
    priv owns_data: bool = false,
    priv owns_id: bool = false,

    pub fn isDone(): bool {
        return std.mem.eql(u8, _.data, "[DONE]");
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.owns_event_type && _.event_type.len > 0) {
            allocator.free(_.event_type);
        }
        if (_.owns_data && _.data.len > 0) {
            allocator.free(_.data);
        }
        if (_.owns_id && _.id.len > 0) {
            allocator.free(_.id);
        }
        _.event_type = "message";
        _.data = "";
        _.id = "";
        _.retry_ms = null;
        _.owns_event_type = false;
        _.owns_data = false;
        _.owns_id = false;
    }
}

priv struct SseByteBuf {
    data: []u8 = [],
    len: usize = 0,

    fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.data.len > 0) {
            allocator.free(_.data);
        }
        _.data = [];
        _.len = 0;
    }

    fn appendBytes(*_, allocator: std.mem.Allocator, src: []const u8): ! {
        if (src.len == 0) return;
        const needed = _.len + src.len;
        if (needed > _.data.len) {
            let new_cap = if (_.data.len == 0) 256 else _.data.len * 2;
            while (new_cap < needed) {
                new_cap *= 2;
            }
            if (_.data.len == 0) {
                _.data = try allocator.alloc(u8, new_cap);
            } else {
                _.data = try allocator.realloc(_.data, new_cap);
            }
        }
        @memcpy(_.data[_.len .. _.len + src.len], src);
        _.len += src.len;
    }

    fn appendByte(*_, allocator: std.mem.Allocator, b: u8): ! {
        const arr: [1]u8 = [b];
        try _.appendBytes(allocator, &arr);
    }

    fn slice(): []const u8 {
        return _.data[0.._.len];
    }

    fn toOwnedSlice(*_, allocator: std.mem.Allocator): ([]const u8)! {
        if (_.len == 0) {
            if (_.data.len > 0) allocator.free(_.data);
            _.data = [];
            return "";
        }
        const res = try allocator.realloc(_.data, _.len);
        _.data = [];
        _.len = 0;
        return res;
    }
}

priv struct SseEventList {
    items: []SseEvent = [],
    len: usize = 0,
    cap: usize = 0,

    fn deinitAll(*_, allocator: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            _.items[i].deinit(allocator);
        }
        if (_.cap > 0) {
            allocator.free(_.items);
        }
        _.items = [];
        _.len = 0;
        _.cap = 0;
    }

    fn push(*_, allocator: std.mem.Allocator, ev: SseEvent): ! {
        if (_.len >= _.cap) {
            const new_cap: usize = if (_.cap == 0) 8 else _.cap * 2;
            if (_.cap == 0) {
                _.items = try allocator.alloc(SseEvent, new_cap);
            } else {
                _.items = try allocator.realloc(_.items, new_cap);
            }
            _.cap = new_cap;
        }
        _.items[_.len] = ev;
        _.len += 1;
    }

    fn toOwnedSlice(*_, allocator: std.mem.Allocator): ([]SseEvent)! {
        if (_.len == 0) {
            if (_.cap > 0) allocator.free(_.items);
            _.items = [];
            _.cap = 0;
            return [];
        }
        if (_.len < _.cap) {
            const shrunk = try allocator.realloc(_.items, _.len);
            _.items = [];
            _.cap = 0;
            _.len = 0;
            return shrunk;
        }
        const full_slice = _.items[0.._.len];
        _.items = [];
        _.cap = 0;
        _.len = 0;
        return full_slice;
    }
}

fn sseParseUint64(s: []const u8): u64? {
    if (s.len == 0) return null;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] < '0' || s[i] > '9') return null;
    }
    return std.fmt.parseInt(u64, s, 10) catch null;
}

fn sseHasCrLfOrNul(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '\r' || s[i] == '\n' || s[i] == 0) return true;
    }
    return false;
}

pub struct SseParser {
    pub allocator: std.mem.Allocator = undefined,
    pub max_event_bytes: usize = DEFAULT_MAX_SSE_EVENT_BYTES,
    priv line_buf: SseByteBuf = SseByteBuf{},
    priv data_buf: SseByteBuf = SseByteBuf{},
    priv has_data: bool = false,
    priv event_type_buf: SseByteBuf = SseByteBuf{},
    priv last_event_id_buf: SseByteBuf = SseByteBuf{},
    priv retry_ms: u64? = null,
    priv pending_cr: bool = false,
    priv bom_state: u8 = 0,
    priv current_event_bytes: usize = 0,

    pub static fn init(allocator: std.mem.Allocator): SseParser {
        return SseParser.initWithMaxSize(allocator, DEFAULT_MAX_SSE_EVENT_BYTES);
    }

    pub static fn initWithMaxSize(allocator: std.mem.Allocator, max_event_bytes: usize): SseParser {
        return SseParser{
            allocator: allocator,
            max_event_bytes: if (max_event_bytes == 0) DEFAULT_MAX_SSE_EVENT_BYTES else max_event_bytes,
            line_buf: SseByteBuf{},
            data_buf: SseByteBuf{},
            has_data: false,
            event_type_buf: SseByteBuf{},
            last_event_id_buf: SseByteBuf{},
            retry_ms: null,
            pending_cr: false,
            bom_state: 0,
            current_event_bytes: 0,
        };
    }

    pub fn deinit(*_) {
        _.line_buf.deinit(_.allocator);
        _.data_buf.deinit(_.allocator);
        _.event_type_buf.deinit(_.allocator);
        _.last_event_id_buf.deinit(_.allocator);
        _.has_data = false;
        _.retry_ms = null;
        _.pending_cr = false;
        _.bom_state = 0;
        _.current_event_bytes = 0;
    }

    pub fn reset(*_) {
        _.line_buf.len = 0;
        _.data_buf.len = 0;
        _.has_data = false;
        _.event_type_buf.len = 0;
        _.last_event_id_buf.len = 0;
        _.retry_ms = null;
        _.pending_cr = false;
        _.bom_state = 0;
        _.current_event_bytes = 0;
    }

    pub fn lastEventId(): []const u8 {
        return _.last_event_id_buf.slice();
    }

    pub fn freeEvents(*_, events: []SseEvent) {
        let i: usize = 0;
        while (i < events.len) : (i += 1) {
            events[i].deinit(_.allocator);
        }
        if (events.len > 0) {
            _.allocator.free(events);
        }
    }

    fn resetEventDraft(*_) {
        _.data_buf.len = 0;
        _.has_data = false;
        _.event_type_buf.len = 0;
        _.retry_ms = null;
        _.current_event_bytes = 0;
    }

    fn chargeEventBytes(*_, extra: usize): ! {
        if (_.current_event_bytes + extra > _.max_event_bytes) {
            return error.SseEventTooLarge;
        }
        _.current_event_bytes += extra;
    }

    fn emitCurrentEvent(*_, out_events: *SseEventList): ! {
        if (!_.has_data || _.data_buf.len == 0) {
            _.resetEventDraft();
            return;
        }

        const data_copy = try _.allocator.dupe(u8, _.data_buf.slice());
        errdefer _.allocator.free(data_copy);

        let ev_type_copy: []const u8 = "message";
        let owns_ev_type = false;
        if (_.event_type_buf.len > 0) {
            ev_type_copy = try _.allocator.dupe(u8, _.event_type_buf.slice());
            owns_ev_type = true;
        }
        errdefer if (owns_ev_type) _.allocator.free(ev_type_copy);

        let id_copy: []const u8 = "";
        let owns_id = false;
        if (_.last_event_id_buf.len > 0) {
            id_copy = try _.allocator.dupe(u8, _.last_event_id_buf.slice());
            owns_id = true;
        }
        errdefer if (owns_id) _.allocator.free(id_copy);

        try out_events.push(_.allocator, SseEvent{
            event_type: ev_type_copy,
            data: data_copy,
            id: id_copy,
            retry_ms: _.retry_ms,
            owns_event_type: owns_ev_type,
            owns_data: true,
            owns_id: owns_id,
        });
        _.resetEventDraft();
    }

    fn processLine(*_, line: []const u8, out_events: *SseEventList): ! {
        if (line.len == 0) {
            try _.emitCurrentEvent(out_events);
            return;
        }
        if (line[0] == ':') {
            return;
        }

        let field: []const u8 = line;
        let value: []const u8 = line[line.len..];
        if (colon := std.mem.indexOfScalar(u8, line, ':')) {
            field = line[0..colon];
            const raw_val = line[colon + 1 ..];
            value = if (raw_val.len > 0 && raw_val[0] == ' ') raw_val[1..] else raw_val;
        }

        if (std.mem.eql(u8, field, "data")) {
            const extra = value.len + (if (_.has_data) 1 as usize else 0 as usize);
            try _.chargeEventBytes(extra);
            if (_.has_data) {
                try _.data_buf.appendByte(_.allocator, '\n');
            }
            try _.data_buf.appendBytes(_.allocator, value);
            _.has_data = true;
        } else if (std.mem.eql(u8, field, "event")) {
            try _.chargeEventBytes(value.len);
            _.event_type_buf.len = 0;
            try _.event_type_buf.appendBytes(_.allocator, value);
        } else if (std.mem.eql(u8, field, "id")) {
            if (std.mem.indexOfScalar(u8, value, 0) == null) {
                try _.chargeEventBytes(value.len);
                _.last_event_id_buf.len = 0;
                try _.last_event_id_buf.appendBytes(_.allocator, value);
            }
        } else if (std.mem.eql(u8, field, "retry")) {
            if (parsed := sseParseUint64(value)) {
                _.retry_ms = parsed;
            }
        }
    }

    fn feedByte(*_, b: u8, out_events: *SseEventList): ! {
        if (_.pending_cr) {
            _.pending_cr = false;
            if (b == '\n') return;
        }

        if (b == '\r') {
            const line_len = _.line_buf.len;
            _.line_buf.len = 0;
            _.pending_cr = true;
            try _.processLine(_.line_buf.data[0..line_len], out_events);
            return;
        }
        if (b == '\n') {
            const line_len = _.line_buf.len;
            _.line_buf.len = 0;
            try _.processLine(_.line_buf.data[0..line_len], out_events);
            return;
        }

        if (_.line_buf.len + 1 + _.current_event_bytes > _.max_event_bytes) {
            return error.SseEventTooLarge;
        }
        try _.line_buf.appendByte(_.allocator, b);
    }

    /// Incrementally feeds a raw byte chunk into the SSE state machine and returns any completed events.
    /// Caller owns the returned slice and must free it via `parser.freeEvents(events)`.
    pub fn feed(*_, chunk: []const u8): ([]SseEvent)! {
        let out_events = SseEventList{};
        errdefer out_events.deinitAll(_.allocator);

        let i: usize = 0;
        while (i < chunk.len) : (i += 1) {
            const b = chunk[i];
            if (_.bom_state < 3) {
                if (_.bom_state == 0) {
                    if (b == 0xEF) {
                        _.bom_state = 1;
                        continue;
                    }
                    _.bom_state = 3;
                } else if (_.bom_state == 1) {
                    if (b == 0xBB) {
                        _.bom_state = 2;
                        continue;
                    }
                    _.bom_state = 3;
                    try _.feedByte(0xEF, &out_events);
                } else if (_.bom_state == 2) {
                    _.bom_state = 3;
                    if (b == 0xBF) {
                        continue;
                    }
                    try _.feedByte(0xEF, &out_events);
                    try _.feedByte(0xBB, &out_events);
                }
            }

            try _.feedByte(b, &out_events);
        }

        return try out_events.toOwnedSlice(_.allocator);
    }
}

pub struct SseEncoder {
    /// Formats a `SseEvent` into wire SSE text (`event:`, `id:`, `retry:`, multi-line `data:`, trailing `\n`).
    pub static fn formatEvent(alloc: std.mem.Allocator, ev: SseEvent): ([]const u8)! {
        if (sseHasCrLfOrNul(ev.event_type) || sseHasCrLfOrNul(ev.id)) {
            return error.InvalidSseField;
        }

        let out = SseByteBuf{};
        errdefer out.deinit(alloc);

        if (ev.id.len > 0) {
            try out.appendBytes(alloc, "id: ");
            try out.appendBytes(alloc, ev.id);
            try out.appendByte(alloc, '\n');
        }

        if (ev.event_type.len > 0 && !std.mem.eql(u8, ev.event_type, "message")) {
            try out.appendBytes(alloc, "event: ");
            try out.appendBytes(alloc, ev.event_type);
            try out.appendByte(alloc, '\n');
        }

        if (r := ev.retry_ms) {
            let num_buf: [32]u8 = undefined;
            const r_str = std.fmt.bufPrint(&num_buf, "{d}", .{r}) catch return error.FormatFailed;
            try out.appendBytes(alloc, "retry: ");
            try out.appendBytes(alloc, r_str);
            try out.appendByte(alloc, '\n');
        }

        let pos: usize = 0;
        if (ev.data.len == 0) {
            try out.appendBytes(alloc, "data: \n");
        } else {
            while (true) {
                let end = pos;
                while (end < ev.data.len && ev.data[end] != '\n' && ev.data[end] != '\r') : (end += 1) {}
                try out.appendBytes(alloc, "data: ");
                try out.appendBytes(alloc, ev.data[pos..end]);
                try out.appendByte(alloc, '\n');

                if (end >= ev.data.len) break;
                if (ev.data[end] == '\r' && end + 1 < ev.data.len && ev.data[end + 1] == '\n') {
                    pos = end + 2;
                } else {
                    pos = end + 1;
                }
                if (pos == ev.data.len) {
                    try out.appendBytes(alloc, "data: \n");
                    break;
                }
            }
        }

        try out.appendByte(alloc, '\n');
        return try out.toOwnedSlice(alloc);
    }

    /// Formats an SSE comment (`: <text>\n`), splitting multi-line text safely.
    pub static fn formatComment(alloc: std.mem.Allocator, text: []const u8): ([]const u8)! {
        let out = SseByteBuf{};
        errdefer out.deinit(alloc);

        if (text.len == 0) {
            try out.appendBytes(alloc, ":\n");
            return try out.toOwnedSlice(alloc);
        }

        let pos: usize = 0;
        while (true) {
            let end = pos;
            while (end < text.len && text[end] != '\n' && text[end] != '\r') : (end += 1) {}
            try out.appendBytes(alloc, ": ");
            try out.appendBytes(alloc, text[pos..end]);
            try out.appendByte(alloc, '\n');

            if (end >= text.len) break;
            if (text[end] == '\r' && end + 1 < text.len && text[end + 1] == '\n') {
                pos = end + 2;
            } else {
                pos = end + 1;
            }
            if (pos == text.len) {
                try out.appendBytes(alloc, ": \n");
                break;
            }
        }

        return try out.toOwnedSlice(alloc);
    }
}
