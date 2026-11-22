// Source: cp/lib/sip/sip.cp (RFC 3261 SIP Wire Message Parser, Compact Headers & Builder)

const std = @import("std");

/// Errors returned by `cap:sip` parsing, validation, and message building operations.
pub err SipError {
    InvalidVersion,
    InvalidStartLine,
    MalformedHeader,
    InvalidContentLength,
    TruncatedBody,
    HeaderInjection,
    OutOfMemory,
}

/// Standard RFC 3261 / extension SIP request methods.
pub enum SipMethod: u8 {
    Invite,
    Ack,
    Bye,
    Cancel,
    Options,
    Register,
    Update,
    Info,
    Notify,
    Subscribe,
    Message,
    Other,

    /// Parses a SIP method token into a `SipMethod` enum value.
    pub static fn fromString(s: []const u8): SipMethod {
        if (std.mem.eql(u8, s, "INVITE")) return .Invite;
        if (std.mem.eql(u8, s, "ACK")) return .Ack;
        if (std.mem.eql(u8, s, "BYE")) return .Bye;
        if (std.mem.eql(u8, s, "CANCEL")) return .Cancel;
        if (std.mem.eql(u8, s, "OPTIONS")) return .Options;
        if (std.mem.eql(u8, s, "REGISTER")) return .Register;
        if (std.mem.eql(u8, s, "UPDATE")) return .Update;
        if (std.mem.eql(u8, s, "INFO")) return .Info;
        if (std.mem.eql(u8, s, "NOTIFY")) return .Notify;
        if (std.mem.eql(u8, s, "SUBSCRIBE")) return .Subscribe;
        if (std.mem.eql(u8, s, "MESSAGE")) return .Message;
        return .Other;
    }

    /// Returns the canonical uppercase SIP method string for known methods.
    pub fn toString(*_): []const u8 {
        return switch (_.*) {
            .Invite => "INVITE",
            .Ack => "ACK",
            .Bye => "BYE",
            .Cancel => "CANCEL",
            .Options => "OPTIONS",
            .Register => "REGISTER",
            .Update => "UPDATE",
            .Info => "INFO",
            .Notify => "NOTIFY",
            .Subscribe => "SUBSCRIBE",
            .Message => "MESSAGE",
            .Other => "OTHER",
        };
    }
}

/// Normalizes RFC 3261 §7.3.3 single-character compact header names to their canonical form.
/// Non-compact header names are returned unchanged.
pub fn normalizeHeaderName(name: []const u8): []const u8 {
    if (name.len == 1) {
        const ch = std.ascii.toLower(name[0]);
        if (ch == 'v') return "Via";
        if (ch == 'f') return "From";
        if (ch == 't') return "To";
        if (ch == 'i') return "Call-ID";
        if (ch == 'm') return "Contact";
        if (ch == 'c') return "Content-Type";
        if (ch == 'l') return "Content-Length";
        if (ch == 's') return "Subject";
        if (ch == 'k') return "Supported";
    }
    return name;
}

/// Represents a single SIP header name-value pair.
pub class SipHeader {
    pub name: []const u8,
    pub value: []const u8,
}

/// Request start-line fields (`Method SP Request-URI SP SIP-Version`).
pub class SipRequestLine {
    pub method: []const u8,
    pub uri: []const u8,
    pub version: []const u8,
}

/// Response status-line fields (`SIP-Version SP Status-Code SP Reason-Phrase`).
pub class SipResponseLine {
    pub version: []const u8,
    pub status_code: u16,
    pub reason: []const u8,
}

/// Tagged union representing either a SIP Request-Line or a SIP Status-Line.
pub enum SipStartLine {
    request: SipRequestLine,
    response: SipResponseLine,
}

type StrSlice = []const u8;
type MutStrSlice = []u8;
const EMPTY_U8_SLICES: [0][]u8 = [];

/// Represents a parsed or constructed RFC 3261 SIP message.
pub class SipMessage {
    pub start_line: SipStartLine,
    pub headers: []SipHeader,
    pub body: []const u8,
    owned_values: [][]u8 = @constCast(&EMPTY_U8_SLICES),

    /// Releases all heap-allocated header slices and folded-header buffers owned by this message.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.owned_values.len) : (i += 1) {
            allocator.free(_.owned_values[i]);
        }
        if (_.owned_values.len > 0) {
            allocator.free(_.owned_values);
        }
        if (_.headers.len > 0) {
            allocator.free(_.headers);
        }
        _.owned_values = unsafe { @constCast(&EMPTY_U8_SLICES) };
        _.headers = _.headers[0..0];
    }

    /// Returns the first header value matching `name` (case-insensitive, compact aliases supported), or `null`.
    pub fn header(name: []const u8): ([]const u8)? {
        const target = normalizeHeaderName(name);
        let i: usize = 0;
        while (i < _.headers.len) : (i += 1) {
            const h_name = normalizeHeaderName(_.headers[i].name);
            if (std.ascii.eqlIgnoreCase(h_name, target)) {
                return _.headers[i].value;
            }
        }
        return null;
    }

    /// Collects all header values matching `name` (case-insensitive, compact aliases supported).
    /// Caller owns the returned slice and must free it with `allocator.free(...)`.
    pub fn headersAll(allocator: std.mem.Allocator, name: []const u8): ([][]const u8) ! SipError {
        const target = normalizeHeaderName(name);
        let list: std.ArrayList(StrSlice) = .empty;
        errdefer list.deinit(allocator);

        let i: usize = 0;
        while (i < _.headers.len) : (i += 1) {
            const h_name = normalizeHeaderName(_.headers[i].name);
            if (std.ascii.eqlIgnoreCase(h_name, target)) {
                list.append(allocator, _.headers[i].value) catch return SipError.OutOfMemory;
            }
        }
        return list.toOwnedSlice(allocator) catch return SipError.OutOfMemory;
    }

    /// Returns the `Call-ID` header value (supports compact `i:` header), or `null`.
    pub fn callId(): ([]const u8)? {
        return _.header("Call-ID");
    }

    /// Returns the `CSeq` header value, or `null`.
    pub fn cseq(): ([]const u8)? {
        return _.header("CSeq");
    }

    /// Returns the topmost `Via` header entry (supports compact `v:` header and comma-separated values), or `null`.
    pub fn viaTop(): ([]const u8)? {
        const first = _.header("Via") orelse return null;
        if (comma := std.mem.indexOfScalar(u8, first, ',')) {
            return std.mem.trim(u8, first[0..comma], " \t");
        }
        return std.mem.trim(u8, first, " \t");
    }

    /// Serializes this `SipMessage` into a wire-format byte buffer with `\r\n` line endings.
    /// Caller owns the returned slice and must free it with `allocator.free(...)`.
    pub fn format(allocator: std.mem.Allocator): ([]u8) ! SipError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);

        switch (_.start_line) {
            .request => |req| {
                out.appendSlice(allocator, req.method) catch return SipError.OutOfMemory;
                out.append(allocator, ' ') catch return SipError.OutOfMemory;
                out.appendSlice(allocator, req.uri) catch return SipError.OutOfMemory;
                out.append(allocator, ' ') catch return SipError.OutOfMemory;
                out.appendSlice(allocator, req.version) catch return SipError.OutOfMemory;
                out.appendSlice(allocator, "\r\n") catch return SipError.OutOfMemory;
            },
            .response => |resp| {
                out.appendSlice(allocator, resp.version) catch return SipError.OutOfMemory;
                out.append(allocator, ' ') catch return SipError.OutOfMemory;
                let code_buf: [8]u8 = undefined;
                const code_str = std.fmt.bufPrint(&code_buf, "{d}", .{resp.status_code}) catch return SipError.OutOfMemory;
                out.appendSlice(allocator, code_str) catch return SipError.OutOfMemory;
                out.append(allocator, ' ') catch return SipError.OutOfMemory;
                out.appendSlice(allocator, resp.reason) catch return SipError.OutOfMemory;
                out.appendSlice(allocator, "\r\n") catch return SipError.OutOfMemory;
            },
        }

        let i: usize = 0;
        while (i < _.headers.len) : (i += 1) {
            const h = _.headers[i];
            out.appendSlice(allocator, h.name) catch return SipError.OutOfMemory;
            out.appendSlice(allocator, ": ") catch return SipError.OutOfMemory;
            out.appendSlice(allocator, h.value) catch return SipError.OutOfMemory;
            out.appendSlice(allocator, "\r\n") catch return SipError.OutOfMemory;
        }

        out.appendSlice(allocator, "\r\n") catch return SipError.OutOfMemory;
        if (_.body.len > 0) {
            out.appendSlice(allocator, _.body) catch return SipError.OutOfMemory;
        }

        return out.toOwnedSlice(allocator) catch return SipError.OutOfMemory;
    }
}

fn hasCrLfOrNul(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        if (ch == '\r' || ch == '\n' || ch == 0) {
            return true;
        }
    }
    return false;
}

fn hasWhitespace(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            return true;
        }
    }
    return false;
}

class LineSpan {
    line: []const u8,
    next_pos: usize,
}

fn nextLine(buf: []const u8, pos: usize): LineSpan? {
    if (pos >= buf.len) return null;
    let end = pos;
    while (end < buf.len && buf[end] != '\n') : (end += 1) {}
    let line_end = end;
    if (line_end > pos && buf[line_end - 1] == '\r') {
        line_end -= 1;
    }
    const next_pos = if (end < buf.len) end + 1 else buf.len;
    return LineSpan{
        line: buf[pos..line_end],
        next_pos: next_pos,
    };
}

fn parseStartLine(line: []const u8): SipStartLine ! SipError {
    if (line.len == 0) return SipError.InvalidStartLine;

    if (std.mem.startsWith(u8, line, "SIP/")) {
        const sp1 = std.mem.indexOfScalar(u8, line, ' ') orelse return SipError.InvalidStartLine;
        const version = line[0..sp1];
        if (!std.mem.eql(u8, version, "SIP/2.0")) {
            return SipError.InvalidVersion;
        }
        const after_ver = line[sp1 + 1 ..];
        if (after_ver.len < 3) return SipError.InvalidStartLine;

        let sp2: usize = 0;
        while (sp2 < after_ver.len && after_ver[sp2] != ' ') : (sp2 += 1) {}
        const code_str = after_ver[0..sp2];
        if (code_str.len != 3) return SipError.InvalidStartLine;
        const status_code = std.fmt.parseInt(u16, code_str, 10) catch return SipError.InvalidStartLine;
        if (status_code < 100 || status_code > 699) {
            return SipError.InvalidStartLine;
        }

        const reason = if (sp2 < after_ver.len) after_ver[sp2 + 1 ..] else "";
        return SipStartLine{
            response: SipResponseLine{
                version: version,
                status_code: status_code,
                reason: reason,
            },
        };
    }

    const sp1 = std.mem.indexOfScalar(u8, line, ' ') orelse return SipError.InvalidStartLine;
    const method = line[0..sp1];
    if (method.len == 0 || hasWhitespace(method)) {
        return SipError.InvalidStartLine;
    }

    const rest = line[sp1 + 1 ..];
    const sp2 = std.mem.indexOfScalar(u8, rest, ' ') orelse return SipError.InvalidStartLine;
    const uri = rest[0..sp2];
    if (uri.len == 0 || hasWhitespace(uri)) {
        return SipError.InvalidStartLine;
    }

    const version = rest[sp2 + 1 ..];
    if (!std.mem.eql(u8, version, "SIP/2.0")) {
        return SipError.InvalidVersion;
    }

    return SipStartLine{
        request: SipRequestLine{
            method: method,
            uri: uri,
            version: version,
        },
    };
}

fn parseContentLengthValue(val: []const u8): usize ! SipError {
    const trimmed = std.mem.trim(u8, val, " \t");
    if (trimmed.len == 0) return SipError.InvalidContentLength;
    let i: usize = 0;
    while (i < trimmed.len) : (i += 1) {
        if (trimmed[i] < '0' || trimmed[i] > '9') {
            return SipError.InvalidContentLength;
        }
    }
    return std.fmt.parseInt(usize, trimmed, 10) catch return SipError.InvalidContentLength;
}

/// Parses a raw RFC 3261 SIP wire message (request or response).
/// Supports `\r\n` and `\n` line endings, header continuation lines (folding),
/// compact header names, and strict `SIP/2.0` + `Content-Length` validation.
pub fn parse(allocator: std.mem.Allocator, raw_bytes: []const u8): SipMessage ! SipError {
    const first_span = nextLine(raw_bytes, 0) orelse return SipError.InvalidStartLine;
    const start_line = parseStartLine(first_span.line)?;

    let headers: std.ArrayList(SipHeader) = .empty;
    errdefer headers.deinit(allocator);

    let owned_values: std.ArrayList(MutStrSlice) = .empty;
    errdefer {
        let k: usize = 0;
        while (k < owned_values.items.len) : (k += 1) {
            allocator.free(owned_values.items[k]);
        }
        owned_values.deinit(allocator);
    }

    // Track whether the last header's value is already in owned_values (at its tail).
    let last_is_owned = false;

    let pos = first_span.next_pos;
    let found_empty_line = false;

    while (span := nextLine(raw_bytes, pos)) {
        pos = span.next_pos;
        const line = span.line;
        if (line.len == 0) {
            found_empty_line = true;
            break;
        }

        if (line[0] == ' ' || line[0] == '\t') {
            if (headers.items.len == 0) {
                return SipError.MalformedHeader;
            }
            const cont = std.mem.trim(u8, line, " \t");
            const last_idx = headers.items.len - 1;
            const prev_val = headers.items[last_idx].value;

            const extra_len = if (cont.len > 0) 1 + cont.len else 0;
            let new_buf = allocator.alloc(u8, prev_val.len + extra_len) catch return SipError.OutOfMemory;
            @memcpy(new_buf[0..prev_val.len], prev_val);
            if (cont.len > 0) {
                new_buf[prev_val.len] = ' ';
                @memcpy(new_buf[prev_val.len + 1 ..], cont);
            }

            if (last_is_owned) {
                const old_idx = owned_values.items.len - 1;
                allocator.free(owned_values.items[old_idx]);
                owned_values.items[old_idx] = new_buf;
            } else {
                owned_values.append(allocator, new_buf) catch {
                    allocator.free(new_buf);
                    return SipError.OutOfMemory;
                };
                last_is_owned = true;
            }
            headers.items[last_idx].value = new_buf;
            continue;
        }

        const colon = std.mem.indexOfScalar(u8, line, ':') orelse return SipError.MalformedHeader;
        const raw_name = std.mem.trimRight(u8, line[0..colon], " \t");
        if (raw_name.len == 0 || hasWhitespace(raw_name)) {
            return SipError.MalformedHeader;
        }

        const norm_name = normalizeHeaderName(raw_name);
        const val = std.mem.trim(u8, line[colon + 1 ..], " \t");

        headers.append(allocator, SipHeader{
            name: norm_name,
            value: val,
        }) catch return SipError.OutOfMemory;
        last_is_owned = false;
    }

    const body_slice = if (found_empty_line) raw_bytes[pos..] else raw_bytes[raw_bytes.len..raw_bytes.len];

    // Validate Content-Length if present
    let content_len: usize? = null;
    let hi: usize = 0;
    while (hi < headers.items.len) : (hi += 1) {
        if (std.ascii.eqlIgnoreCase(headers.items[hi].name, "Content-Length")) {
            const parsed_len = parseContentLengthValue(headers.items[hi].value)?;
            content_len = parsed_len;
            break;
        }
    }

    let final_body = body_slice;
    if (cl := content_len) {
        if (body_slice.len < cl) {
            return SipError.TruncatedBody;
        }
        final_body = body_slice[0..cl];
    }

    const headers_slice = headers.toOwnedSlice(allocator) catch return SipError.OutOfMemory;
    const owned_slice = if (owned_values.items.len > 0)
        (owned_values.toOwnedSlice(allocator) catch {
            allocator.free(headers_slice);
            return SipError.OutOfMemory;
        })
    else empty_blk: {
        owned_values.deinit(allocator);
        break :empty_blk unsafe { @constCast(&EMPTY_U8_SLICES) };
    };

    return SipMessage{
        start_line: start_line,
        headers: headers_slice,
        body: final_body,
        owned_values: owned_slice,
    };
}

/// Fluent RFC 3261 SIP request/response wire message builder.
/// Automatically maintains `Content-Length` and blocks CRLF/NUL header injection.
pub class SipBuilder {
    allocator: std.mem.Allocator,
    is_request: bool,
    method: []const u8,
    uri: []const u8,
    status_code: u16,
    reason: []const u8,
    headers: std.ArrayList(SipHeader),
    payload: []const u8,

    /// Initializes a builder for a SIP request (`METHOD uri SIP/2.0`).
    pub static fn request(allocator: std.mem.Allocator, method: []const u8, uri: []const u8): SipBuilder {
        return SipBuilder{
            allocator: allocator,
            is_request: true,
            method: method,
            uri: uri,
            status_code: 0,
            reason: "",
            headers: .empty,
            payload: "",
        };
    }

    /// Initializes a builder for a SIP response (`SIP/2.0 status_code reason`).
    pub static fn response(allocator: std.mem.Allocator, status_code: u16, reason: []const u8): SipBuilder {
        return SipBuilder{
            allocator: allocator,
            is_request: false,
            method: "",
            uri: "",
            status_code: status_code,
            reason: reason,
            headers: .empty,
            payload: "",
        };
    }

    /// Frees internal header list memory held by the builder.
    pub fn deinit(*_) {
        _.headers.deinit(_.allocator);
    }

    /// Appends a header after checking for CRLF/NUL injection in both `name` and `value`.
    pub fn header(*_, name: []const u8, value: []const u8): (*SipBuilder) ! SipError {
        if (name.len == 0 || hasWhitespace(name) || hasCrLfOrNul(name) || hasCrLfOrNul(value)) {
            return SipError.HeaderInjection;
        }
        _.headers.append(_.allocator, SipHeader{
            name: name,
            value: value,
        }) catch return SipError.OutOfMemory;
        return _;
    }

    /// Sets the SIP message body (e.g. SDP payload).
    pub fn setBody(*_, body_bytes: []const u8): *SipBuilder {
        _.payload = body_bytes;
        return _;
    }

    /// Serializes the built SIP message into wire format, automatically emitting `Content-Length`.
    pub fn build(*_): ([]u8) ! SipError {
        if (_.is_request) {
            if (_.method.len == 0 || hasWhitespace(_.method) || hasCrLfOrNul(_.method) ||
                _.uri.len == 0 || hasWhitespace(_.uri) || hasCrLfOrNul(_.uri))
            {
                return SipError.HeaderInjection;
            }
        } else {
            if (_.status_code < 100 || _.status_code > 699) {
                return SipError.InvalidStartLine;
            }
            if (hasCrLfOrNul(_.reason)) {
                return SipError.HeaderInjection;
            }
        }

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(_.allocator);

        if (_.is_request) {
            out.appendSlice(_.allocator, _.method) catch return SipError.OutOfMemory;
            out.append(_.allocator, ' ') catch return SipError.OutOfMemory;
            out.appendSlice(_.allocator, _.uri) catch return SipError.OutOfMemory;
            out.appendSlice(_.allocator, " SIP/2.0\r\n") catch return SipError.OutOfMemory;
        } else {
            out.appendSlice(_.allocator, "SIP/2.0 ") catch return SipError.OutOfMemory;
            let code_buf: [8]u8 = undefined;
            const code_str = std.fmt.bufPrint(&code_buf, "{d}", .{_.status_code}) catch return SipError.OutOfMemory;
            out.appendSlice(_.allocator, code_str) catch return SipError.OutOfMemory;
            out.append(_.allocator, ' ') catch return SipError.OutOfMemory;
            out.appendSlice(_.allocator, _.reason) catch return SipError.OutOfMemory;
            out.appendSlice(_.allocator, "\r\n") catch return SipError.OutOfMemory;
        }

        let has_content_length = false;
        let i: usize = 0;
        while (i < _.headers.items.len) : (i += 1) {
            const h = _.headers.items[i];
            if (std.ascii.eqlIgnoreCase(normalizeHeaderName(h.name), "Content-Length")) {
                has_content_length = true;
            }
            out.appendSlice(_.allocator, h.name) catch return SipError.OutOfMemory;
            out.appendSlice(_.allocator, ": ") catch return SipError.OutOfMemory;
            out.appendSlice(_.allocator, h.value) catch return SipError.OutOfMemory;
            out.appendSlice(_.allocator, "\r\n") catch return SipError.OutOfMemory;
        }

        if (!has_content_length) {
            out.appendSlice(_.allocator, "Content-Length: ") catch return SipError.OutOfMemory;
            let len_buf: [32]u8 = undefined;
            const len_str = std.fmt.bufPrint(&len_buf, "{d}", .{_.payload.len}) catch return SipError.OutOfMemory;
            out.appendSlice(_.allocator, len_str) catch return SipError.OutOfMemory;
            out.appendSlice(_.allocator, "\r\n") catch return SipError.OutOfMemory;
        }

        out.appendSlice(_.allocator, "\r\n") catch return SipError.OutOfMemory;
        if (_.payload.len > 0) {
            out.appendSlice(_.allocator, _.payload) catch return SipError.OutOfMemory;
        }

        return out.toOwnedSlice(_.allocator) catch return SipError.OutOfMemory;
    }
}
