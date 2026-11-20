// Source: cp/lib/sse/sse.cp (Pure CP W3C Server-Sent Events Incremental Parser & Encoder)
// Zero-C-dependency WHATWG/W3C text/event-stream incremental streaming parser and frame-safe encoder.

const std = @import("std");

/// Unified error set for `cap:sse` streaming parser and encoder operations.
pub err SseError {
    EventBufferOverflow,
    InvalidRetryValue,
    InvalidFieldName,
    InvalidEventData,
    OutOfMemory,
}

/// A single decoded Server-Sent Event frame.
/// All string slices (`event_type`, `data`, and optional `id`) are heap-allocated
/// and owned by the caller, who must call `deinit(allocator)` when done.
pub struct SseEvent {
    event_type: []const u8,
    data: []const u8,
    id: ([]const u8)?,
    retry_ms: u64?,

    /// Releases `event_type`, `data`, and `id` (if present) using `allocator`.
    pub fn deinit(allocator: std.mem.Allocator) {
        allocator.free(_.event_type);
        allocator.free(_.data);
        if (id_val := _.id) {
            allocator.free(id_val);
        }
    }
}

fn containsCrOrLf(bytes: []const u8): bool {
    for (b of bytes) {
        if (b == '\r' || b == '\n') {
            return true;
        }
    }
    return false;
}

fn containsNul(bytes: []const u8): bool {
    for (b of bytes) {
        if (b == 0) {
            return true;
        }
    }
    return false;
}

fn parseAsciiU64(bytes: []const u8): u64? {
    if (bytes.len == 0) return null;
    for (b of bytes) {
        if (b < '0' || b > '9') {
            return null;
        }
    }
    return std.fmt.parseInt(u64, bytes, 10) catch return null;
}

fn writeU64(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: u64): void ! SseError {
    let buf: [24]u8 = [0; 24];
    const rendered = std.fmt.bufPrint(&buf, "{d}", .{val}) catch return SseError.OutOfMemory;
    out.appendSlice(alloc, rendered) catch return SseError.OutOfMemory;
}

/// Incremental WHATWG/W3C `text/event-stream` parser.
/// Supports arbitrary chunk boundaries, optional leading UTF-8 BOM stripping,
/// `\r\n` / `\n` / `\r` line endings (including `\r` and `\n` split across chunks),
/// single leading space stripping after `:`, multi-line `data:` concatenation,
/// `Last-Event-ID` tracking (ignoring values containing `\x00`), `retry:` tracking,
/// and strict `max_event_bytes` resource-exhaustion protection.
pub struct SseParser {
    alloc: std.mem.Allocator,
    max_event_bytes: usize,
    bom_checked: bool = false,
    bom_buf: [3]u8 = [0; 3],
    bom_len: usize = 0,
    pending_cr: bool = false,
    line_buf: std.ArrayList(u8) = .empty,
    data_buf: std.ArrayList(u8) = .empty,
    event_type_buf: std.ArrayList(u8) = .empty,
    last_event_id: std.ArrayList(u8) = .empty,
    has_id: bool = false,
    current_retry_ms: u64? = null,
    events: std.ArrayList(SseEvent) = .empty,
    event_cursor: usize = 0,

    /// Initializes a new `SseParser` with the given allocator and per-event byte limit.
    pub static fn init(allocator: std.mem.Allocator, max_event_bytes: usize): SseParser {
        return SseParser{
            alloc: allocator,
            max_event_bytes: max_event_bytes,
            bom_checked: false,
            bom_buf: [0; 3],
            bom_len: 0,
            pending_cr: false,
            line_buf: .empty,
            data_buf: .empty,
            event_type_buf: .empty,
            last_event_id: .empty,
            has_id: false,
            current_retry_ms: null,
            events: .empty,
            event_cursor: 0,
        };
    }

    /// Releases all internal buffers and any unconsumed queued `SseEvent` instances.
    pub fn deinit(*_) {
        let i: usize = _.event_cursor;
        while (i < _.events.items.len) : (i += 1) {
            _.events.items[i].deinit(_.alloc);
        }
        _.events.deinit(_.alloc);
        _.line_buf.deinit(_.alloc);
        _.data_buf.deinit(_.alloc);
        _.event_type_buf.deinit(_.alloc);
        _.last_event_id.deinit(_.alloc);
    }

    fn dispatchEvent(*_): void ! SseError {
        if (_.data_buf.items.len == 0) {
            _.event_type_buf.clearRetainingCapacity();
            return;
        }

        let data_len = _.data_buf.items.len;
        if (data_len > 0 && _.data_buf.items[data_len - 1] == '\n') {
            data_len -= 1;
        }

        const owned_data = _.alloc.dupe(u8, _.data_buf.items[0..data_len]) catch return SseError.OutOfMemory;
        const type_src: []const u8 = if (_.event_type_buf.items.len > 0) _.event_type_buf.items else "message";
        const owned_type = _.alloc.dupe(u8, type_src) catch {
            _.alloc.free(owned_data);
            return SseError.OutOfMemory;
        };

        let owned_id: ([]const u8)? = null;
        if (_.has_id) {
            owned_id = _.alloc.dupe(u8, _.last_event_id.items) catch {
                _.alloc.free(owned_data);
                _.alloc.free(owned_type);
                return SseError.OutOfMemory;
            };
        }

        _.events.append(_.alloc, SseEvent{
            event_type: owned_type,
            data: owned_data,
            id: owned_id,
            retry_ms: _.current_retry_ms,
        }) catch {
            _.alloc.free(owned_data);
            _.alloc.free(owned_type);
            if (id_val := owned_id) {
                _.alloc.free(id_val);
            }
            return SseError.OutOfMemory;
        };

        _.data_buf.clearRetainingCapacity();
        _.event_type_buf.clearRetainingCapacity();
    }

    fn processLine(*_, line: []const u8): void ! SseError {
        if (line.len == 0) {
            _.dispatchEvent()?;
            return;
        }

        // Comment line starting with ':'
        if (line[0] == ':') {
            return;
        }

        let field: []const u8 = line;
        let value: []const u8 = "";

        let colon_pos: usize? = null;
        let i: usize = 0;
        while (i < line.len) : (i += 1) {
            if (line[i] == ':') {
                colon_pos = i;
                break;
            }
        }

        if (cpos := colon_pos) {
            field = line[0..cpos];
            let raw_val = line[(cpos + 1)..line.len];
            // Strip at most one leading U+0020 SPACE
            if (raw_val.len > 0 && raw_val[0] == ' ') {
                raw_val = raw_val[1..raw_val.len];
            }
            value = raw_val;
        }

        if (std.mem.eql(u8, field, "event")) {
            if (value.len > _.max_event_bytes) {
                return SseError.EventBufferOverflow;
            }
            _.event_type_buf.clearRetainingCapacity();
            _.event_type_buf.appendSlice(_.alloc, value) catch return SseError.OutOfMemory;
        } else if (std.mem.eql(u8, field, "data")) {
            if (_.data_buf.items.len + value.len + 1 > _.max_event_bytes) {
                return SseError.EventBufferOverflow;
            }
            _.data_buf.appendSlice(_.alloc, value) catch return SseError.OutOfMemory;
            _.data_buf.append(_.alloc, '\n') catch return SseError.OutOfMemory;
        } else if (std.mem.eql(u8, field, "id")) {
            // W3C spec: ignore id field if value contains U+0000 NULL
            if (!containsNul(value)) {
                if (value.len > _.max_event_bytes) {
                    return SseError.EventBufferOverflow;
                }
                _.last_event_id.clearRetainingCapacity();
                _.last_event_id.appendSlice(_.alloc, value) catch return SseError.OutOfMemory;
                _.has_id = true;
            }
        } else if (std.mem.eql(u8, field, "retry")) {
            if (parsed := parseAsciiU64(value)) {
                _.current_retry_ms = parsed;
            }
        }
    }

    fn processByte(*_, b: u8): void ! SseError {
        if (_.pending_cr) {
            _.pending_cr = false;
            if (b == '\n') {
                return;
            }
        }

        if (b == '\r') {
            _.processLine(_.line_buf.items)?;
            _.line_buf.clearRetainingCapacity();
            _.pending_cr = true;
            return;
        }

        if (b == '\n') {
            _.processLine(_.line_buf.items)?;
            _.line_buf.clearRetainingCapacity();
            return;
        }

        if (_.data_buf.items.len + _.line_buf.items.len + 1 > _.max_event_bytes) {
            return SseError.EventBufferOverflow;
        }
        _.line_buf.append(_.alloc, b) catch return SseError.OutOfMemory;
    }

    /// Feeds an arbitrary-length byte chunk into the incremental SSE parser.
    pub fn feed(*_, chunk: []const u8): void ! SseError {
        let idx: usize = 0;

        if (!_.bom_checked) {
            const bom: [3]u8 = [0xEF, 0xBB, 0xBF];
            while (idx < chunk.len && _.bom_len < 3) {
                const b = chunk[idx];
                if (b == bom[_.bom_len]) {
                    _.bom_buf[_.bom_len] = b;
                    _.bom_len += 1;
                    idx += 1;
                    if (_.bom_len == 3) {
                        _.bom_len = 0;
                        _.bom_checked = true;
                        break;
                    }
                } else {
                    _.bom_checked = true;
                    let bi: usize = 0;
                    while (bi < _.bom_len) : (bi += 1) {
                        _.processByte(_.bom_buf[bi])?;
                    }
                    _.bom_len = 0;
                    break;
                }
            }
        }

        while (idx < chunk.len) : (idx += 1) {
            _.processByte(chunk[idx])?;
        }
    }

    /// Pops and returns the next parsed `SseEvent` from the internal queue, or `null` if none are ready.
    /// Caller owns the returned `SseEvent` and must call `ev.deinit(allocator)`.
    pub fn nextEvent(*_): SseEvent? {
        if (_.event_cursor >= _.events.items.len) {
            return null;
        }
        const ev = _.events.items[_.event_cursor];
        _.event_cursor += 1;
        if (_.event_cursor == _.events.items.len) {
            _.events.clearRetainingCapacity();
            _.event_cursor = 0;
        }
        return ev;
    }

    /// Returns the most recent valid `Last-Event-ID` seen by the parser (empty slice if none).
    pub fn lastEventId(): []const u8 {
        return _.last_event_id.items;
    }

    /// Returns the most recent valid `retry:` reconnection interval in milliseconds, or `null`.
    pub fn retryMs(): u64? {
        return _.current_retry_ms;
    }
}

/// W3C `text/event-stream` frame-safe encoder.
pub struct SseEncoder {
    /// Formats a single SSE event frame with optional `event_type`, multi-line `data` splitting,
    /// optional `id`, and optional `retry_ms`.
    /// Rejects `event_type` or `id` containing `\r` or `\n` with `SseError.InvalidEventData`.
    pub static fn formatEvent(
        allocator: std.mem.Allocator,
        event_type: ([]const u8)?,
        data: []const u8,
        id: ([]const u8)?,
        retry_ms: u64?,
    ): []u8 ! SseError {
        if (et := event_type) {
            if (containsCrOrLf(et)) {
                return SseError.InvalidEventData;
            }
        }
        if (id_val := id) {
            if (containsCrOrLf(id_val)) {
                return SseError.InvalidEventData;
            }
        }

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);

        if (et := event_type) {
            if (et.len > 0) {
                out.appendSlice(allocator, "event: ") catch return SseError.OutOfMemory;
                out.appendSlice(allocator, et) catch return SseError.OutOfMemory;
                out.append(allocator, '\n') catch return SseError.OutOfMemory;
            }
        }

        if (id_val := id) {
            out.appendSlice(allocator, "id: ") catch return SseError.OutOfMemory;
            out.appendSlice(allocator, id_val) catch return SseError.OutOfMemory;
            out.append(allocator, '\n') catch return SseError.OutOfMemory;
        }

        if (r_val := retry_ms) {
            out.appendSlice(allocator, "retry: ") catch return SseError.OutOfMemory;
            writeU64(&out, allocator, r_val)?;
            out.append(allocator, '\n') catch return SseError.OutOfMemory;
        }

        // Split data by '\n' into multiple "data: ...\n" lines
        let start: usize = 0;
        let i: usize = 0;
        while (i <= data.len) : (i += 1) {
            if (i == data.len || data[i] == '\n') {
                out.appendSlice(allocator, "data: ") catch return SseError.OutOfMemory;
                out.appendSlice(allocator, data[start..i]) catch return SseError.OutOfMemory;
                out.append(allocator, '\n') catch return SseError.OutOfMemory;
                start = i + 1;
            }
        }

        // Blank line event terminator
        out.append(allocator, '\n') catch return SseError.OutOfMemory;
        return out.toOwnedSlice(allocator) catch return SseError.OutOfMemory;
    }

    /// Formats an SSE comment line (`: <text>\n`).
    /// Rejects `text` containing `\r` or `\n` with `SseError.InvalidEventData`.
    pub static fn formatComment(allocator: std.mem.Allocator, text: []const u8): []u8 ! SseError {
        if (containsCrOrLf(text)) {
            return SseError.InvalidEventData;
        }
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);

        out.appendSlice(allocator, ": ") catch return SseError.OutOfMemory;
        out.appendSlice(allocator, text) catch return SseError.OutOfMemory;
        out.append(allocator, '\n') catch return SseError.OutOfMemory;
        return out.toOwnedSlice(allocator) catch return SseError.OutOfMemory;
    }
}
