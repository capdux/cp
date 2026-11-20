// Source: cp/lib/stomp/stomp.cp (Pure CP STOMP 1.2 Wire Protocol Framing & Incremental Parser)
// Zero-C-dependency STOMP 1.2 frame encoder, header escaper, builder, and streaming parser.

const std = @import("std");

/// Unified error set for `cap:stomp` framing, escaping, and incremental parsing.
pub err StompError {
    InvalidCommand,
    InvalidHeader,
    InvalidEscape,
    InvalidContentLength,
    BodyTooLarge,
    TruncatedFrame,
    OutOfMemory,
}

/// STOMP 1.2 client and server frame commands.
pub enum StompCommand {
    Connect,
    Stomp,
    Send,
    Subscribe,
    Unsubscribe,
    Ack,
    Nack,
    Begin,
    Commit,
    Abort,
    Disconnect,
    Connected,
    Message,
    Receipt,
    Error,

    /// Returns the wire-protocol ASCII command name.
    pub fn toString(): []const u8 {
        return switch (_) {
            .Connect => "CONNECT",
            .Stomp => "STOMP",
            .Send => "SEND",
            .Subscribe => "SUBSCRIBE",
            .Unsubscribe => "UNSUBSCRIBE",
            .Ack => "ACK",
            .Nack => "NACK",
            .Begin => "BEGIN",
            .Commit => "COMMIT",
            .Abort => "ABORT",
            .Disconnect => "DISCONNECT",
            .Connected => "CONNECTED",
            .Message => "MESSAGE",
            .Receipt => "RECEIPT",
            .Error => "ERROR",
        };
    }

    /// Parses a wire-protocol command line into a `StompCommand`.
    pub static fn parse(s: []const u8): StompCommand ! StompError {
        if (std.mem.eql(u8, s, "CONNECT")) return StompCommand.Connect;
        if (std.mem.eql(u8, s, "STOMP")) return StompCommand.Stomp;
        if (std.mem.eql(u8, s, "SEND")) return StompCommand.Send;
        if (std.mem.eql(u8, s, "SUBSCRIBE")) return StompCommand.Subscribe;
        if (std.mem.eql(u8, s, "UNSUBSCRIBE")) return StompCommand.Unsubscribe;
        if (std.mem.eql(u8, s, "ACK")) return StompCommand.Ack;
        if (std.mem.eql(u8, s, "NACK")) return StompCommand.Nack;
        if (std.mem.eql(u8, s, "BEGIN")) return StompCommand.Begin;
        if (std.mem.eql(u8, s, "COMMIT")) return StompCommand.Commit;
        if (std.mem.eql(u8, s, "ABORT")) return StompCommand.Abort;
        if (std.mem.eql(u8, s, "DISCONNECT")) return StompCommand.Disconnect;
        if (std.mem.eql(u8, s, "CONNECTED")) return StompCommand.Connected;
        if (std.mem.eql(u8, s, "MESSAGE")) return StompCommand.Message;
        if (std.mem.eql(u8, s, "RECEIPT")) return StompCommand.Receipt;
        if (std.mem.eql(u8, s, "ERROR")) return StompCommand.Error;
        return StompError.InvalidCommand;
    }

    /// Per STOMP 1.2 specification, CONNECT and CONNECTED frames do not apply
    /// backslash header value escaping for backward compatibility with STOMP 1.0.
    pub fn skipsHeaderEscaping(): bool {
        return _ == StompCommand.Connect || _ == StompCommand.Connected;
    }
}

/// A single STOMP header key-value pair.
pub struct StompHeader {
    name: []const u8,
    value: []const u8,
}

/// A decoded or outbound STOMP 1.2 frame.
/// When returned by `StompStreamParser.nextFrame()`, all header strings,
/// the `headers` slice, and `body` are heap-allocated and must be freed
/// via `frame.deinit(allocator)`.
pub struct StompFrame {
    command: StompCommand,
    headers: []StompHeader,
    body: []const u8,

    /// Releases all heap-allocated header names, header values, the headers slice, and body.
    pub fn deinit(allocator: std.mem.Allocator) {
        for (h of _.headers) {
            allocator.free(h.name);
            allocator.free(h.value);
        }
        allocator.free(_.headers);
        allocator.free(_.body);
    }

    /// Returns the first header value matching `name` per STOMP 1.2 repeated-header rules.
    pub fn header(name: []const u8): ([]const u8)? {
        for (h of _.headers) {
            if (std.mem.eql(u8, h.name, name)) {
                return h.value;
            }
        }
        return null;
    }

    /// Convenience accessor for the `content-type` header.
    pub fn contentType(): ([]const u8)? {
        return _.header("content-type");
    }

    /// Convenience accessor for the `subscription` header.
    pub fn subscription(): ([]const u8)? {
        return _.header("subscription");
    }

    /// Convenience accessor for the `message-id` header.
    pub fn messageId(): ([]const u8)? {
        return _.header("message-id");
    }
}

fn isValidHeaderName(name: []const u8): bool {
    if (name.len == 0) return false;
    for (b of name) {
        if (b == ':' || b == '\r' || b == '\n') {
            return false;
        }
    }
    return true;
}

fn containsCrOrLf(bytes: []const u8): bool {
    for (b of bytes) {
        if (b == '\r' || b == '\n') {
            return true;
        }
    }
    return false;
}

fn parseContentLength(raw: []const u8): usize ! StompError {
    if (raw.len == 0) return StompError.InvalidContentLength;
    for (b of raw) {
        if (b < '0' || b > '9') {
            return StompError.InvalidContentLength;
        }
    }
    return std.fmt.parseInt(usize, raw, 10) catch return StompError.InvalidContentLength;
}

/// Escapes a STOMP 1.2 header value:
/// `\r` -> `\r`, `\n` -> `\n`, `:` -> `\c`, `\` -> `\\`.
pub fn escapeHeaderValue(allocator: std.mem.Allocator, raw: []const u8): []u8 ! StompError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    for (b of raw) {
        if (b == '\r') {
            out.appendSlice(allocator, "\\r") catch return StompError.OutOfMemory;
        } else if (b == '\n') {
            out.appendSlice(allocator, "\\n") catch return StompError.OutOfMemory;
        } else if (b == ':') {
            out.appendSlice(allocator, "\\c") catch return StompError.OutOfMemory;
        } else if (b == '\\') {
            out.appendSlice(allocator, "\\\\") catch return StompError.OutOfMemory;
        } else {
            out.append(allocator, b) catch return StompError.OutOfMemory;
        }
    }

    return out.toOwnedSlice(allocator) catch return StompError.OutOfMemory;
}

/// Unescapes a STOMP 1.2 header value:
/// `\r` -> `\r`, `\n` -> `\n`, `\c` -> `:`, `\\` -> `\`.
/// Returns `StompError.InvalidEscape` on any unrecognized escape sequence or trailing `\`.
pub fn unescapeHeaderValue(allocator: std.mem.Allocator, escaped: []const u8): []u8 ! StompError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    let i: usize = 0;
    while (i < escaped.len) {
        const b = escaped[i];
        if (b == '\\') {
            if (i + 1 >= escaped.len) {
                return StompError.InvalidEscape;
            }
            const next = escaped[i + 1];
            if (next == 'r') {
                out.append(allocator, '\r') catch return StompError.OutOfMemory;
            } else if (next == 'n') {
                out.append(allocator, '\n') catch return StompError.OutOfMemory;
            } else if (next == 'c') {
                out.append(allocator, ':') catch return StompError.OutOfMemory;
            } else if (next == '\\') {
                out.append(allocator, '\\') catch return StompError.OutOfMemory;
            } else {
                return StompError.InvalidEscape;
            }
            i += 2;
        } else {
            out.append(allocator, b) catch return StompError.OutOfMemory;
            i += 1;
        }
    }

    return out.toOwnedSlice(allocator) catch return StompError.OutOfMemory;
}

/// Encodes a `StompFrame` into STOMP 1.2 wire format terminated by `\x00`.
/// Automatically injects `content-length` when `frame.body.len > 0` and no explicit
/// `content-length` header is present. Validates header names and escapes header values
/// (except on `CONNECT` and `CONNECTED` frames per STOMP 1.2).
pub fn encodeFrame(allocator: std.mem.Allocator, frame: StompFrame): []u8 ! StompError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    out.appendSlice(allocator, frame.command.toString()) catch return StompError.OutOfMemory;
    out.append(allocator, '\n') catch return StompError.OutOfMemory;

    const skip_escape = frame.command.skipsHeaderEscaping();
    let has_content_length = false;

    for (h of frame.headers) {
        if (!isValidHeaderName(h.name)) {
            return StompError.InvalidHeader;
        }
        if (std.mem.eql(u8, h.name, "content-length")) {
            has_content_length = true;
        }

        out.appendSlice(allocator, h.name) catch return StompError.OutOfMemory;
        out.append(allocator, ':') catch return StompError.OutOfMemory;

        if (skip_escape) {
            if (containsCrOrLf(h.value)) {
                return StompError.InvalidHeader;
            }
            out.appendSlice(allocator, h.value) catch return StompError.OutOfMemory;
        } else {
            const escaped = escapeHeaderValue(allocator, h.value)?;
            defer allocator.free(escaped);
            out.appendSlice(allocator, escaped) catch return StompError.OutOfMemory;
        }

        out.append(allocator, '\n') catch return StompError.OutOfMemory;
    }

    if (frame.body.len > 0 && !has_content_length) {
        let len_buf: [24]u8 = [0; 24];
        const len_str = std.fmt.bufPrint(&len_buf, "{d}", .{frame.body.len}) catch return StompError.OutOfMemory;
        out.appendSlice(allocator, "content-length:") catch return StompError.OutOfMemory;
        out.appendSlice(allocator, len_str) catch return StompError.OutOfMemory;
        out.append(allocator, '\n') catch return StompError.OutOfMemory;
    }

    out.append(allocator, '\n') catch return StompError.OutOfMemory;
    if (frame.body.len > 0) {
        out.appendSlice(allocator, frame.body) catch return StompError.OutOfMemory;
    }
    out.append(allocator, 0) catch return StompError.OutOfMemory;

    return out.toOwnedSlice(allocator) catch return StompError.OutOfMemory;
}

/// Ergonomic builder for constructing and encoding STOMP 1.2 frames.
pub struct StompBuilder {
    alloc: std.mem.Allocator,
    command: StompCommand,
    headers: std.ArrayList(StompHeader) = .empty,
    body_buf: std.ArrayList(u8) = .empty,

    /// Initializes a new `StompBuilder` for `command`.
    pub static fn init(allocator: std.mem.Allocator, command: StompCommand): StompBuilder {
        return StompBuilder{
            alloc: allocator,
            command: command,
            headers: .empty,
            body_buf: .empty,
        };
    }

    /// Frees all buffered headers and body data.
    pub fn deinit(*_) {
        for (h of _.headers.items) {
            _.alloc.free(h.name);
            _.alloc.free(h.value);
        }
        _.headers.deinit(_.alloc);
        _.body_buf.deinit(_.alloc);
    }

    /// Appends a header to the frame after validating the header name.
    pub fn addHeader(*_, name: []const u8, value: []const u8): void ! StompError {
        if (!isValidHeaderName(name)) {
            return StompError.InvalidHeader;
        }
        const dup_name = _.alloc.dupe(u8, name) catch return StompError.OutOfMemory;
        errdefer _.alloc.free(dup_name);
        const dup_val = _.alloc.dupe(u8, value) catch return StompError.OutOfMemory;
        errdefer _.alloc.free(dup_val);

        _.headers.append(_.alloc, StompHeader{
            name: dup_name,
            value: dup_val,
        }) catch return StompError.OutOfMemory;
    }

    /// Replaces the frame body with a copy of `body`.
    pub fn setBody(*_, body: []const u8): void ! StompError {
        _.body_buf.clearRetainingCapacity();
        _.body_buf.appendSlice(_.alloc, body) catch return StompError.OutOfMemory;
    }

    /// Encodes the current builder state into STOMP 1.2 wire bytes.
    /// Caller owns the returned slice and must free it with `allocator.free(...)`.
    pub fn build(): []u8 ! StompError {
        const frame = StompFrame{
            command: _.command,
            headers: _.headers.items,
            body: _.body_buf.items,
        };
        return encodeFrame(_.alloc, frame);
    }
}

enum ParserState {
    ReadCommand,
    ReadHeaders,
    ReadBodyLen,
    ReadBodyNul,
}

/// Incremental STOMP 1.2 wire-protocol stream parser.
/// Handles arbitrary chunk boundaries, inter-frame heartbeat EOLs (`\n` / `\r\n`),
/// `\r\n` and `\n` line endings, `content-length` binary bodies with embedded `\x00`,
/// NUL-delimited bodies without `content-length`, and `max_body_bytes` overflow guards.
pub struct StompStreamParser {
    alloc: std.mem.Allocator,
    max_body_bytes: usize,
    state: ParserState = ParserState.ReadCommand,
    buf: std.ArrayList(u8) = .empty,
    pos: usize = 0,
    current_cmd: StompCommand = StompCommand.Connect,
    current_headers: std.ArrayList(StompHeader) = .empty,
    content_length: usize? = null,
    saw_content_length: bool = false,
    frames: std.ArrayList(StompFrame) = .empty,
    frame_cursor: usize = 0,

    /// Initializes a new `StompStreamParser` with a maximum allowed body size in bytes.
    pub static fn init(allocator: std.mem.Allocator, max_body_bytes: usize): StompStreamParser {
        return StompStreamParser{
            alloc: allocator,
            max_body_bytes: max_body_bytes,
            state: ParserState.ReadCommand,
            buf: .empty,
            pos: 0,
            current_cmd: StompCommand.Connect,
            current_headers: .empty,
            content_length: null,
            saw_content_length: false,
            frames: .empty,
            frame_cursor: 0,
        };
    }

    fn clearCurrentHeaders(*_) {
        for (h of _.current_headers.items) {
            _.alloc.free(h.name);
            _.alloc.free(h.value);
        }
        _.current_headers.clearRetainingCapacity();
    }

    /// Releases all internal buffers, partial headers, and unconsumed frames.
    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
        _.clearCurrentHeaders();
        _.current_headers.deinit(_.alloc);
        let i = _.frame_cursor;
        while (i < _.frames.items.len) : (i += 1) {
            _.frames.items[i].deinit(_.alloc);
        }
        _.frames.deinit(_.alloc);
    }

    fn findByte(haystack: []const u8, needle: u8): usize? {
        let i: usize = 0;
        while (i < haystack.len) : (i += 1) {
            if (haystack[i] == needle) {
                return i;
            }
        }
        return null;
    }

    fn emitFrame(*_, body_slice: []const u8): void ! StompError {
        const dup_body = _.alloc.dupe(u8, body_slice) catch return StompError.OutOfMemory;
        errdefer _.alloc.free(dup_body);

        const owned_headers = _.current_headers.toOwnedSlice(_.alloc) catch return StompError.OutOfMemory;
        errdefer {
            for (h of owned_headers) {
                _.alloc.free(h.name);
                _.alloc.free(h.value);
            }
            _.alloc.free(owned_headers);
        }

        _.frames.append(_.alloc, StompFrame{
            command: _.current_cmd,
            headers: owned_headers,
            body: dup_body,
        }) catch return StompError.OutOfMemory;

        _.content_length = null;
        _.saw_content_length = false;
        _.state = ParserState.ReadCommand;
    }

    /// Feeds an arbitrary chunk of wire bytes into the parser.
    /// Complete frames become available via `nextFrame()`.
    pub fn feed(*_, chunk: []const u8): void ! StompError {
        if (chunk.len > 0) {
            _.buf.appendSlice(_.alloc, chunk) catch return StompError.OutOfMemory;
        }

        while (true) {
            if (_.state == ParserState.ReadCommand) {
                // Skip inter-frame heartbeat EOLs (\n or \r\n)
                while (_.pos < _.buf.items.len) {
                    const b = _.buf.items[_.pos];
                    if (b == '\n') {
                        _.pos += 1;
                    } else if (b == '\r') {
                        if (_.pos + 1 >= _.buf.items.len) {
                            // Need next byte to see if this is \r\n
                            break;
                        }
                        if (_.buf.items[_.pos + 1] == '\n') {
                            _.pos += 2;
                        } else {
                            return StompError.InvalidCommand;
                        }
                    } else {
                        break;
                    }
                }

                if (_.pos >= _.buf.items.len) {
                    break;
                }
                if (_.buf.items[_.pos] == '\r') {
                    // Lone trailing \r waiting for \n
                    break;
                }

                const rem = _.buf.items[_.pos..];
                const nl_opt = StompStreamParser.findByte(rem, '\n');
                if (nl_idx := nl_opt) {
                    let line = rem[0..nl_idx];
                    if (line.len > 0 && line[line.len - 1] == '\r') {
                        line = line[0 .. line.len - 1];
                    }
                    _.pos += nl_idx + 1;

                    const cmd = StompCommand.parse(line)?;
                    _.current_cmd = cmd;
                    _.clearCurrentHeaders();
                    _.content_length = null;
                    _.saw_content_length = false;
                    _.state = ParserState.ReadHeaders;
                } else {
                    break;
                }
            } else if (_.state == ParserState.ReadHeaders) {
                const rem = _.buf.items[_.pos..];
                const nl_opt = StompStreamParser.findByte(rem, '\n');
                if (nl_idx := nl_opt) {
                    let line = rem[0..nl_idx];
                    if (line.len > 0 && line[line.len - 1] == '\r') {
                        line = line[0 .. line.len - 1];
                    }
                    _.pos += nl_idx + 1;

                    if (line.len == 0) {
                        // End of headers
                        if (cl := _.content_length) {
                            if (cl > _.max_body_bytes) {
                                return StompError.BodyTooLarge;
                            }
                            _.state = ParserState.ReadBodyLen;
                        } else {
                            _.state = ParserState.ReadBodyNul;
                        }
                    } else {
                        const colon_opt = StompStreamParser.findByte(line, ':');
                        if (colon_idx := colon_opt) {
                            const raw_name = line[0..colon_idx];
                            if (!isValidHeaderName(raw_name)) {
                                return StompError.InvalidHeader;
                            }
                            const raw_val = line[colon_idx + 1 ..];

                            let decoded_val: []u8 = undefined;
                            if (_.current_cmd.skipsHeaderEscaping()) {
                                decoded_val = _.alloc.dupe(u8, raw_val) catch return StompError.OutOfMemory;
                            } else {
                                decoded_val = unescapeHeaderValue(_.alloc, raw_val)?;
                            }
                            errdefer _.alloc.free(decoded_val);

                            if (std.mem.eql(u8, raw_name, "content-length") && !_.saw_content_length) {
                                const parsed_cl = parseContentLength(decoded_val)?;
                                _.content_length = parsed_cl;
                                _.saw_content_length = true;
                            }

                            const dup_name = _.alloc.dupe(u8, raw_name) catch return StompError.OutOfMemory;
                            errdefer _.alloc.free(dup_name);

                            _.current_headers.append(_.alloc, StompHeader{
                                name: dup_name,
                                value: decoded_val,
                            }) catch return StompError.OutOfMemory;
                        } else {
                            return StompError.InvalidHeader;
                        }
                    }
                } else {
                    break;
                }
            } else if (_.state == ParserState.ReadBodyLen) {
                if (expected_len := _.content_length) {
                    const rem = _.buf.items[_.pos..];
                    if (rem.len < expected_len + 1) {
                        break;
                    }
                    if (rem[expected_len] != 0) {
                        return StompError.TruncatedFrame;
                    }
                    _.emitFrame(rem[0..expected_len])?;
                    _.pos += expected_len + 1;
                } else {
                    return StompError.InvalidContentLength;
                }
            } else {
                // ParserState.ReadBodyNul
                const rem = _.buf.items[_.pos..];
                const nul_opt = StompStreamParser.findByte(rem, 0);
                if (nul_idx := nul_opt) {
                    if (nul_idx > _.max_body_bytes) {
                        return StompError.BodyTooLarge;
                    }
                    _.emitFrame(rem[0..nul_idx])?;
                    _.pos += nul_idx + 1;
                } else {
                    if (rem.len > _.max_body_bytes) {
                        return StompError.BodyTooLarge;
                    }
                    break;
                }
            }
        }

        // Compact consumed prefix
        if (_.pos >= _.buf.items.len) {
            _.buf.clearRetainingCapacity();
            _.pos = 0;
        } else if (_.pos > 0) {
            const remaining = _.buf.items.len - _.pos;
            std.mem.copyForwards(u8, _.buf.items[0..remaining], _.buf.items[_.pos..]);
            _.buf.items.len = remaining;
            _.pos = 0;
        }
    }

    /// Pops and returns the next decoded `StompFrame`, or `null` if none is ready.
    /// Caller owns the returned frame and must call `frame.deinit(allocator)`.
    pub fn nextFrame(*_): StompFrame? {
        if (_.frame_cursor >= _.frames.items.len) {
            return null;
        }
        const f = _.frames.items[_.frame_cursor];
        _.frame_cursor += 1;
        if (_.frame_cursor == _.frames.items.len) {
            _.frames.clearRetainingCapacity();
            _.frame_cursor = 0;
        }
        return f;
    }
}
