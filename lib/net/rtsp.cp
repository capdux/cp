// Source: cp/lib/rtsp/rtsp.cp (Pure CP RFC 2326 RTSP Protocol Codec, Transport Header & Interleaved '$' Framing)
// Zero-dependency RFC 2326 RTSP/1.0 message builder, Transport header parser/formatter,
// and incremental stream parser supporting mixed text messages and '$' interleaved RTP/RTCP frames.

const std = @import("std");

/// Default maximum payload size (1 MiB) for `RtspStreamParser`.
pub const DEFAULT_MAX_PAYLOAD_BYTES: usize = 1048576;

/// Unified error set for `cap:rtsp`.
pub err RtspError {
    InvalidVersion,
    InvalidStartLine,
    MalformedHeader,
    InvalidContentLength,
    InvalidTransport,
    InvalidInterleavedFrame,
    PayloadTooLarge,
    HeaderInjection,
    OutOfMemory,
}

/// RFC 2326 RTSP/1.0 request methods.
pub enum RtspMethod {
    Options,
    Describe,
    Announce,
    Setup,
    Play,
    Pause,
    Teardown,
    GetParameter,
    SetParameter,
    Record,
    Redirect,

    /// Returns the uppercase ASCII wire method name.
    pub fn toString(): []const u8 {
        return switch (_) {
            .Options => "OPTIONS",
            .Describe => "DESCRIBE",
            .Announce => "ANNOUNCE",
            .Setup => "SETUP",
            .Play => "PLAY",
            .Pause => "PAUSE",
            .Teardown => "TEARDOWN",
            .GetParameter => "GET_PARAMETER",
            .SetParameter => "SET_PARAMETER",
            .Record => "RECORD",
            .Redirect => "REDIRECT",
        };
    }

    /// Parses an uppercase RTSP method token.
    pub static fn parse(s: []const u8): RtspMethod ! RtspError {
        if (std.mem.eql(u8, s, "OPTIONS")) return RtspMethod.Options;
        if (std.mem.eql(u8, s, "DESCRIBE")) return RtspMethod.Describe;
        if (std.mem.eql(u8, s, "ANNOUNCE")) return RtspMethod.Announce;
        if (std.mem.eql(u8, s, "SETUP")) return RtspMethod.Setup;
        if (std.mem.eql(u8, s, "PLAY")) return RtspMethod.Play;
        if (std.mem.eql(u8, s, "PAUSE")) return RtspMethod.Pause;
        if (std.mem.eql(u8, s, "TEARDOWN")) return RtspMethod.Teardown;
        if (std.mem.eql(u8, s, "GET_PARAMETER")) return RtspMethod.GetParameter;
        if (std.mem.eql(u8, s, "SET_PARAMETER")) return RtspMethod.SetParameter;
        if (std.mem.eql(u8, s, "RECORD")) return RtspMethod.Record;
        if (std.mem.eql(u8, s, "REDIRECT")) return RtspMethod.Redirect;
        return RtspError.InvalidStartLine;
    }
}

/// A single RTSP header name-value pair.
pub struct RtspHeader {
    name: []const u8,
    value: []const u8,
}

/// Parsed RTSP request start-line: `<METHOD> <URI> <VERSION>`.
pub struct RtspRequestLine {
    method: RtspMethod,
    uri: []const u8,
    version: []const u8,
}

/// Parsed RTSP response start-line: `<VERSION> <STATUS_CODE> <REASON_PHRASE>`.
pub struct RtspResponseLine {
    version: []const u8,
    status_code: u16,
    reason_phrase: []const u8,
}

/// Start-line union for an RTSP/1.0 message.
pub enum RtspStartLine {
    request: RtspRequestLine,
    response: RtspResponseLine,
}

fn containsCrOrLf(bytes: []const u8): bool {
    for (b of bytes) {
        if (b == '\r' || b == '\n') {
            return true;
        }
    }
    return false;
}

fn isValidHeaderName(name: []const u8): bool {
    if (name.len == 0) return false;
    for (b of name) {
        if (b == ':' || b == '\r' || b == '\n' || b == ' ' || b == '\t') {
            return false;
        }
    }
    return true;
}

fn trimAscii(s: []const u8): []const u8 {
    return std.mem.trim(u8, s, " \t");
}

fn parseDecimalUsize(raw: []const u8): usize ! RtspError {
    if (raw.len == 0) return RtspError.InvalidContentLength;
    for (b of raw) {
        if (b < '0' || b > '9') {
            return RtspError.InvalidContentLength;
        }
    }
    return std.fmt.parseInt(usize, raw, 10) catch return RtspError.InvalidContentLength;
}

fn parseDecimalU16(raw: []const u8): u16 ! RtspError {
    if (raw.len == 0) return RtspError.InvalidTransport;
    for (b of raw) {
        if (b < '0' || b > '9') {
            return RtspError.InvalidTransport;
        }
    }
    return std.fmt.parseInt(u16, raw, 10) catch return RtspError.InvalidTransport;
}

/// A decoded or constructed RTSP/1.0 message (Request or Response).
/// All string fields, headers, and body are heap-allocated and freed via `deinit(allocator)`.
pub struct RtspMessage {
    start_line: RtspStartLine,
    headers: []RtspHeader,
    body: []const u8,

    /// Releases all heap-allocated slices owned by this message.
    pub fn deinit(allocator: std.mem.Allocator) {
        switch (_.start_line) {
            .request r => {
                if (r.uri.len > 0) allocator.free(r.uri);
                if (r.version.len > 0) allocator.free(r.version);
            },
            .response r => {
                if (r.version.len > 0) allocator.free(r.version);
                if (r.reason_phrase.len > 0) allocator.free(r.reason_phrase);
            },
        }
        for (h of _.headers) {
            if (h.name.len > 0) allocator.free(h.name);
            if (h.value.len > 0) allocator.free(h.value);
        }
        if (_.headers.len > 0) {
            allocator.free(_.headers);
        }
        if (_.body.len > 0) {
            allocator.free(_.body);
        }
    }

    /// Case-insensitive lookup for the first header matching `name`.
    pub fn header(name: []const u8): ([]const u8)? {
        for (h of _.headers) {
            if (std.ascii.eqlIgnoreCase(h.name, name)) {
                return h.value;
            }
        }
        return null;
    }

    /// Parses and returns the `CSeq` header as `u32`, or `null` if absent or non-numeric.
    pub fn cseq(): u32? {
        const val = _.header("CSeq") orelse return null;
        const trimmed = trimAscii(val);
        if (trimmed.len == 0) return null;
        return std.fmt.parseInt(u32, trimmed, 10) catch return null;
    }

    /// Returns the `Session` identifier (stripping any `;timeout=...` parameter), or `null`.
    pub fn sessionId(): ([]const u8)? {
        for (h of _.headers) {
            if (std.ascii.eqlIgnoreCase(h.name, "Session")) {
                const semi_opt = std.mem.indexOfScalar(u8, h.value, ';');
                if (semi := semi_opt) {
                    const id = trimAscii(h.value[0..semi]);
                    if (id.len == 0) return null;
                    return id;
                }
                const id = trimAscii(h.value);
                if (id.len == 0) return null;
                return id;
            }
        }
        return null;
    }

    /// Returns the `Content-Base` header value if present.
    pub fn contentBase(): ([]const u8)? {
        return _.header("Content-Base");
    }

    /// Serializes this message into RFC 2326 wire format (`\r\n` line endings).
    /// Automatically injects `Content-Length` when `body.len > 0` and no `Content-Length` header exists.
    /// Rejects CR/LF characters inside start-line fields or headers with `RtspError.HeaderInjection`.
    pub fn format(allocator: std.mem.Allocator): []u8 ! RtspError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);

        switch (_.start_line) {
            .request r => {
                if (r.uri.len == 0 || containsCrOrLf(r.uri) || containsCrOrLf(r.version)) {
                    return RtspError.HeaderInjection;
                }
                if (!std.mem.eql(u8, r.version, "RTSP/1.0")) {
                    return RtspError.InvalidVersion;
                }
                out.appendSlice(allocator, r.method.toString()) catch return RtspError.OutOfMemory;
                out.append(allocator, ' ') catch return RtspError.OutOfMemory;
                out.appendSlice(allocator, r.uri) catch return RtspError.OutOfMemory;
                out.append(allocator, ' ') catch return RtspError.OutOfMemory;
                out.appendSlice(allocator, r.version) catch return RtspError.OutOfMemory;
                out.appendSlice(allocator, "\r\n") catch return RtspError.OutOfMemory;
            },
            .response r => {
                if (containsCrOrLf(r.version) || containsCrOrLf(r.reason_phrase)) {
                    return RtspError.HeaderInjection;
                }
                if (!std.mem.eql(u8, r.version, "RTSP/1.0")) {
                    return RtspError.InvalidVersion;
                }
                let code_buf: [16]u8 = undefined;
                const code_str = std.fmt.bufPrint(&code_buf, "{d}", .{r.status_code}) catch return RtspError.InvalidStartLine;
                out.appendSlice(allocator, r.version) catch return RtspError.OutOfMemory;
                out.append(allocator, ' ') catch return RtspError.OutOfMemory;
                out.appendSlice(allocator, code_str) catch return RtspError.OutOfMemory;
                out.append(allocator, ' ') catch return RtspError.OutOfMemory;
                out.appendSlice(allocator, r.reason_phrase) catch return RtspError.OutOfMemory;
                out.appendSlice(allocator, "\r\n") catch return RtspError.OutOfMemory;
            },
        }

        let has_content_length = false;
        for (h of _.headers) {
            if (containsCrOrLf(h.name) || containsCrOrLf(h.value)) {
                return RtspError.HeaderInjection;
            }
            if (!isValidHeaderName(h.name)) {
                return RtspError.MalformedHeader;
            }
            if (std.ascii.eqlIgnoreCase(h.name, "Content-Length")) {
                has_content_length = true;
            }
            out.appendSlice(allocator, h.name) catch return RtspError.OutOfMemory;
            out.appendSlice(allocator, ": ") catch return RtspError.OutOfMemory;
            out.appendSlice(allocator, h.value) catch return RtspError.OutOfMemory;
            out.appendSlice(allocator, "\r\n") catch return RtspError.OutOfMemory;
        }

        if (_.body.len > 0 && !has_content_length) {
            let len_buf: [32]u8 = undefined;
            const len_str = std.fmt.bufPrint(&len_buf, "{d}", .{_.body.len}) catch return RtspError.OutOfMemory;
            out.appendSlice(allocator, "Content-Length: ") catch return RtspError.OutOfMemory;
            out.appendSlice(allocator, len_str) catch return RtspError.OutOfMemory;
            out.appendSlice(allocator, "\r\n") catch return RtspError.OutOfMemory;
        }

        out.appendSlice(allocator, "\r\n") catch return RtspError.OutOfMemory;
        if (_.body.len > 0) {
            out.appendSlice(allocator, _.body) catch return RtspError.OutOfMemory;
        }

        return out.toOwnedSlice(allocator) catch return RtspError.OutOfMemory;
    }
}

/// A single channel/port or consecutive RTP/RTCP pair (e.g. `5000-5001` or `0-1`).
pub struct RtspPortPair {
    rtp: u16,
    rtcp: u16?,
}

/// Parsed RFC 2326 §12.39 `Transport` header specification.
pub struct TransportSpec {
    protocol: []const u8,
    unicast: bool,
    multicast: bool,
    interleaved: RtspPortPair?,
    client_port: RtspPortPair?,
    server_port: RtspPortPair?,
    ssrc: ([]const u8)?,
    mode: ([]const u8)?,
    destination: ([]const u8)?,
    source: ([]const u8)?,
    ttl: u8?,
    layers: u32?,

    /// Releases heap-allocated string fields when constructed via `parseTransportHeader`.
    pub fn deinit(allocator: std.mem.Allocator) {
        if (_.protocol.len > 0) allocator.free(_.protocol);
        if (s := _.ssrc) {
            if (s.len > 0) allocator.free(s);
        }
        if (m := _.mode) {
            if (m.len > 0) allocator.free(m);
        }
        if (d := _.destination) {
            if (d.len > 0) allocator.free(d);
        }
        if (src := _.source) {
            if (src.len > 0) allocator.free(src);
        }
    }
}

fn parsePortPair(raw: []const u8): RtspPortPair ! RtspError {
    if (raw.len == 0) return RtspError.InvalidTransport;
    const dash_opt = std.mem.indexOfScalar(u8, raw, '-');
    if (dash := dash_opt) {
        const p1 = parseDecimalU16(raw[0..dash])?;
        const p2 = parseDecimalU16(raw[dash + 1 ..])?;
        return RtspPortPair{ rtp: p1, rtcp: p2 };
    }
    const p1 = parseDecimalU16(raw)?;
    return RtspPortPair{ rtp: p1, rtcp: null };
}

fn stripQuotes(raw: []const u8): []const u8 {
    if (raw.len >= 2 && raw[0] == '"' && raw[raw.len - 1] == '"') {
        return raw[1 .. raw.len - 1];
    }
    return raw;
}

/// Parses an RFC 2326 §12.39 `Transport` header value into a `TransportSpec`.
/// Example: `RTP/AVP/TCP;unicast;interleaved=0-1;ssrc=deadbeef`
/// Caller owns heap-allocated strings and must call `spec.deinit(allocator)`.
pub fn parseTransportHeader(allocator: std.mem.Allocator, val: []const u8): TransportSpec ! RtspError {
    const trimmed_all = trimAscii(val);
    if (trimmed_all.len == 0 || containsCrOrLf(trimmed_all)) {
        return RtspError.InvalidTransport;
    }

    let it = std.mem.splitScalar(u8, trimmed_all, ';');
    const first_raw = it.next() orelse return RtspError.InvalidTransport;
    const proto_trimmed = trimAscii(first_raw);
    if (proto_trimmed.len == 0 || std.mem.indexOfScalar(u8, proto_trimmed, '=') != null) {
        return RtspError.InvalidTransport;
    }

    const proto_dup = allocator.dupe(u8, proto_trimmed) catch return RtspError.OutOfMemory;
    let spec = TransportSpec{
        protocol: proto_dup,
        unicast: false,
        multicast: false,
        interleaved: null,
        client_port: null,
        server_port: null,
        ssrc: null,
        mode: null,
        destination: null,
        source: null,
        ttl: null,
        layers: null,
    };
    errdefer spec.deinit(allocator);

    while (part_raw := it.next()) {
        const part = trimAscii(part_raw);
        if (part.len == 0) continue;

        const eq_opt = std.mem.indexOfScalar(u8, part, '=');
        if (eq := eq_opt) {
            const key = trimAscii(part[0..eq]);
            const pval = trimAscii(part[eq + 1 ..]);
            if (key.len == 0 || pval.len == 0) {
                return RtspError.InvalidTransport;
            }
            if (std.ascii.eqlIgnoreCase(key, "interleaved")) {
                const pair = parsePortPair(pval)?;
                if (pair.rtp > 255) return RtspError.InvalidTransport;
                if (rtcp_ch := pair.rtcp) {
                    if (rtcp_ch > 255) return RtspError.InvalidTransport;
                }
                spec.interleaved = pair;
            } else if (std.ascii.eqlIgnoreCase(key, "client_port")) {
                spec.client_port = parsePortPair(pval)?;
            } else if (std.ascii.eqlIgnoreCase(key, "server_port")) {
                spec.server_port = parsePortPair(pval)?;
            } else if (std.ascii.eqlIgnoreCase(key, "ssrc")) {
                if (s := spec.ssrc) allocator.free(s);
                spec.ssrc = allocator.dupe(u8, pval) catch return RtspError.OutOfMemory;
            } else if (std.ascii.eqlIgnoreCase(key, "mode")) {
                const unquoted = stripQuotes(pval);
                if (unquoted.len == 0) return RtspError.InvalidTransport;
                if (m := spec.mode) allocator.free(m);
                spec.mode = allocator.dupe(u8, unquoted) catch return RtspError.OutOfMemory;
            } else if (std.ascii.eqlIgnoreCase(key, "destination")) {
                if (d := spec.destination) allocator.free(d);
                spec.destination = allocator.dupe(u8, pval) catch return RtspError.OutOfMemory;
            } else if (std.ascii.eqlIgnoreCase(key, "source")) {
                if (src := spec.source) allocator.free(src);
                spec.source = allocator.dupe(u8, pval) catch return RtspError.OutOfMemory;
            } else if (std.ascii.eqlIgnoreCase(key, "ttl")) {
                spec.ttl = std.fmt.parseInt(u8, pval, 10) catch return RtspError.InvalidTransport;
            } else if (std.ascii.eqlIgnoreCase(key, "layers")) {
                spec.layers = std.fmt.parseInt(u32, pval, 10) catch return RtspError.InvalidTransport;
            }
        } else {
            if (std.ascii.eqlIgnoreCase(part, "unicast")) {
                if (spec.multicast) return RtspError.InvalidTransport;
                spec.unicast = true;
            } else if (std.ascii.eqlIgnoreCase(part, "multicast")) {
                if (spec.unicast) return RtspError.InvalidTransport;
                spec.multicast = true;
            }
        }
    }

    return spec;
}

fn appendPortPair(allocator: std.mem.Allocator, out: *std.ArrayList(u8), key: []const u8, pair: RtspPortPair): void ! RtspError {
    let buf: [64]u8 = undefined;
    let s: []const u8 = "";
    if (rtcp := pair.rtcp) {
        s = std.fmt.bufPrint(&buf, ";{s}={d}-{d}", .{key, pair.rtp, rtcp}) catch return RtspError.OutOfMemory;
    } else {
        s = std.fmt.bufPrint(&buf, ";{s}={d}", .{key, pair.rtp}) catch return RtspError.OutOfMemory;
    }
    out.appendSlice(allocator, s) catch return RtspError.OutOfMemory;
}

/// Formats a `TransportSpec` into an RFC 2326 `Transport` header value.
pub fn formatTransportHeader(allocator: std.mem.Allocator, spec: TransportSpec): []u8 ! RtspError {
    if (spec.protocol.len == 0 || containsCrOrLf(spec.protocol)) {
        return RtspError.InvalidTransport;
    }
    if (spec.unicast && spec.multicast) {
        return RtspError.InvalidTransport;
    }

    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    out.appendSlice(allocator, spec.protocol) catch return RtspError.OutOfMemory;
    if (spec.unicast) {
        out.appendSlice(allocator, ";unicast") catch return RtspError.OutOfMemory;
    } else if (spec.multicast) {
        out.appendSlice(allocator, ";multicast") catch return RtspError.OutOfMemory;
    }
    if (dst := spec.destination) {
        if (containsCrOrLf(dst)) return RtspError.HeaderInjection;
        out.appendSlice(allocator, ";destination=") catch return RtspError.OutOfMemory;
        out.appendSlice(allocator, dst) catch return RtspError.OutOfMemory;
    }
    if (src := spec.source) {
        if (containsCrOrLf(src)) return RtspError.HeaderInjection;
        out.appendSlice(allocator, ";source=") catch return RtspError.OutOfMemory;
        out.appendSlice(allocator, src) catch return RtspError.OutOfMemory;
    }
    if (pair := spec.interleaved) {
        appendPortPair(allocator, &out, "interleaved", pair)?;
    }
    if (pair := spec.client_port) {
        appendPortPair(allocator, &out, "client_port", pair)?;
    }
    if (pair := spec.server_port) {
        appendPortPair(allocator, &out, "server_port", pair)?;
    }
    if (ssrc := spec.ssrc) {
        if (containsCrOrLf(ssrc)) return RtspError.HeaderInjection;
        out.appendSlice(allocator, ";ssrc=") catch return RtspError.OutOfMemory;
        out.appendSlice(allocator, ssrc) catch return RtspError.OutOfMemory;
    }
    if (mode := spec.mode) {
        if (containsCrOrLf(mode)) return RtspError.HeaderInjection;
        out.appendSlice(allocator, ";mode=\"") catch return RtspError.OutOfMemory;
        out.appendSlice(allocator, mode) catch return RtspError.OutOfMemory;
        out.append(allocator, '"') catch return RtspError.OutOfMemory;
    }
    if (ttl := spec.ttl) {
        let buf: [24]u8 = undefined;
        const s = std.fmt.bufPrint(&buf, ";ttl={d}", .{ttl}) catch return RtspError.OutOfMemory;
        out.appendSlice(allocator, s) catch return RtspError.OutOfMemory;
    }
    if (layers := spec.layers) {
        let buf: [32]u8 = undefined;
        const s = std.fmt.bufPrint(&buf, ";layers={d}", .{layers}) catch return RtspError.OutOfMemory;
        out.appendSlice(allocator, s) catch return RtspError.OutOfMemory;
    }

    return out.toOwnedSlice(allocator) catch return RtspError.OutOfMemory;
}

/// An RFC 2326 §10.12 binary interleaved RTP/RTCP frame: `['$'][channel:u8][length:u16 BE][data]`.
pub struct RtspInterleavedFrame {
    channel: u8,
    data: []const u8,

    /// Frees the frame payload slice.
    pub fn deinit(allocator: std.mem.Allocator) {
        if (_.data.len > 0) {
            allocator.free(_.data);
        }
    }
}

/// Encodes a binary interleaved frame `['$'][channel:u8][length:u16 BE][data]`.
pub fn encodeInterleavedFrame(allocator: std.mem.Allocator, channel: u8, data: []const u8): []u8 ! RtspError {
    if (data.len > 65535) {
        return RtspError.PayloadTooLarge;
    }
    let out = allocator.alloc(u8, 4 + data.len) catch return RtspError.OutOfMemory;
    out[0] = '$';
    out[1] = channel;
    out[2] = ((data.len >> 8) & 0xFF) as u8;
    out[3] = (data.len & 0xFF) as u8;
    if (data.len > 0) {
        std.mem.copyForwards(u8, out[4..], data);
    }
    return out;
}

/// Item yielded by `RtspStreamParser.nextFrame()`.
pub enum RtspStreamItem {
    message: RtspMessage,
    interleaved: RtspInterleavedFrame,

    /// Releases all heap allocations owned by either variant.
    pub fn deinit(allocator: std.mem.Allocator) {
        switch (_) {
            .message m => m.deinit(allocator),
            .interleaved f => f.deinit(allocator),
        }
    }
}

/// Incremental RTSP/1.0 stream parser supporting arbitrary chunk boundaries, sticky packets,
/// `Content-Length` bodies (e.g. SDP), and RFC 2326 §10.12 `$` binary interleaved RTP/RTCP frames.
pub struct RtspStreamParser {
    alloc: std.mem.Allocator,
    buf: std.ArrayList(u8),
    pos: usize,
    max_payload_bytes: usize,

    /// Initializes a stream parser with a maximum payload/body limit in bytes.
    pub static fn init(allocator: std.mem.Allocator, max_payload_bytes: usize): RtspStreamParser {
        return RtspStreamParser{
            alloc: allocator,
            buf: .empty,
            pos: 0,
            max_payload_bytes: max_payload_bytes,
        };
    }

    /// Releases the internal accumulation buffer.
    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
    }

    /// Appends an arbitrary network chunk to the parser buffer.
    pub fn feed(*_, chunk: []const u8): void ! RtspError {
        if (chunk.len == 0) return;
        _.buf.appendSlice(_.alloc, chunk) catch return RtspError.OutOfMemory;
    }

    fn consumeBytes(*_, n: usize) {
        _.pos += n;
        if (_.pos >= _.buf.items.len) {
            _.buf.clearRetainingCapacity();
            _.pos = 0;
        } else if (_.pos >= 4096) {
            const rem = _.buf.items.len - _.pos;
            std.mem.copyForwards(u8, _.buf.items[0..rem], _.buf.items[_.pos..]);
            _.buf.items.len = rem;
            _.pos = 0;
        }
    }

    static fn parseStartLine(alloc: std.mem.Allocator, line: []const u8): RtspStartLine ! RtspError {
        if (line.len == 0) return RtspError.InvalidStartLine;

        if (std.mem.startsWith(u8, line, "RTSP/")) {
            // Response: RTSP/1.0 <3-digit status> [reason]
            const sp1 = std.mem.indexOfScalar(u8, line, ' ') orelse return RtspError.InvalidStartLine;
            const version = line[0..sp1];
            if (!std.mem.eql(u8, version, "RTSP/1.0")) {
                return RtspError.InvalidVersion;
            }
            const rest = trimAscii(line[sp1 + 1 ..]);
            if (rest.len < 3) return RtspError.InvalidStartLine;

            let code_end: usize = 0;
            while (code_end < rest.len && rest[code_end] != ' ') {
                code_end += 1;
            }
            if (code_end != 3) return RtspError.InvalidStartLine;
            const code_str = rest[0..3];
            for (c of code_str) {
                if (c < '0' || c > '9') return RtspError.InvalidStartLine;
            }
            const status_code = std.fmt.parseInt(u16, code_str, 10) catch return RtspError.InvalidStartLine;
            if (status_code < 100 || status_code > 599) return RtspError.InvalidStartLine;

            const reason = if (code_end < rest.len) trimAscii(rest[code_end + 1 ..]) else "";

            const ver_dup = alloc.dupe(u8, version) catch return RtspError.OutOfMemory;
            errdefer alloc.free(ver_dup);
            const reason_dup = if (reason.len > 0)
                (alloc.dupe(u8, reason) catch return RtspError.OutOfMemory)
            else
                "";

            return RtspStartLine{
                response: RtspResponseLine{
                    version: ver_dup,
                    status_code: status_code,
                    reason_phrase: reason_dup,
                },
            };
        }

        // Request: <METHOD> <URI> <VERSION>
        const sp1 = std.mem.indexOfScalar(u8, line, ' ') orelse return RtspError.InvalidStartLine;
        const method_str = line[0..sp1];
        const method = RtspMethod.parse(method_str)?;

        const after_method = trimAscii(line[sp1 + 1 ..]);
        const sp2 = std.mem.indexOfScalar(u8, after_method, ' ') orelse return RtspError.InvalidStartLine;
        const uri = trimAscii(after_method[0..sp2]);
        const version = trimAscii(after_method[sp2 + 1 ..]);
        if (uri.len == 0 || version.len == 0) return RtspError.InvalidStartLine;

        if (!std.mem.startsWith(u8, version, "RTSP/")) {
            return RtspError.InvalidStartLine;
        }
        if (!std.mem.eql(u8, version, "RTSP/1.0")) {
            return RtspError.InvalidVersion;
        }

        const uri_dup = alloc.dupe(u8, uri) catch return RtspError.OutOfMemory;
        errdefer alloc.free(uri_dup);
        const ver_dup = alloc.dupe(u8, version) catch return RtspError.OutOfMemory;

        return RtspStartLine{
            request: RtspRequestLine{
                method: method,
                uri: uri_dup,
                version: ver_dup,
            },
        };
    }

    /// Parses and returns the next complete RTSP message or `$` interleaved frame,
    /// or `null` if more bytes are needed. Caller owns the returned item and must call `deinit(allocator)`.
    pub fn nextFrame(*_): RtspStreamItem? ! RtspError {
        const rem = _.buf.items[_.pos..];
        if (rem.len == 0) return null;

        // RFC 2326 §10.12 Interleaved binary frame: ['$'][channel:u8][length:u16 BE][payload]
        if (rem[0] == '$') {
            if (rem.len < 4) return null;
            const channel = rem[1];
            const payload_len: usize = ((rem[2] as usize) << 8) | (rem[3] as usize);
            if (payload_len > _.max_payload_bytes) {
                return RtspError.PayloadTooLarge;
            }
            if (rem.len < 4 + payload_len) return null;

            const data_dup = if (payload_len > 0)
                (_.alloc.dupe(u8, rem[4 .. 4 + payload_len]) catch return RtspError.OutOfMemory)
            else
                "";
            _.consumeBytes(4 + payload_len);
            return RtspStreamItem{
                interleaved: RtspInterleavedFrame{
                    channel: channel,
                    data: data_dup,
                },
            };
        }

        // Text RTSP/1.0 message: wait for "\r\n\r\n"
        const hdr_end_opt = std.mem.indexOf(u8, rem, "\r\n\r\n");
        let hdr_end: usize = 0;
        if (h := hdr_end_opt) {
            hdr_end = h;
        } else {
            // Even before full headers arrive, check if first line already has an invalid version/start-line
            if (first_nl := std.mem.indexOf(u8, rem, "\r\n")) {
                const probe = RtspStreamParser.parseStartLine(_.alloc, rem[0..first_nl])?;
                switch (probe) {
                    .request r => {
                        if (r.uri.len > 0) _.alloc.free(r.uri);
                        if (r.version.len > 0) _.alloc.free(r.version);
                    },
                    .response r => {
                        if (r.version.len > 0) _.alloc.free(r.version);
                        if (r.reason_phrase.len > 0) _.alloc.free(r.reason_phrase);
                    },
                }
            }
            if (rem.len > _.max_payload_bytes) {
                return RtspError.PayloadTooLarge;
            }
            return null;
        }

        const header_block = rem[0..hdr_end];
        const first_crlf = std.mem.indexOf(u8, header_block, "\r\n");
        let start_line_raw: []const u8 = header_block;
        if (idx := first_crlf) {
            start_line_raw = header_block[0..idx];
        }

        const start_line = RtspStreamParser.parseStartLine(_.alloc, start_line_raw)?;
        let free_on_incomplete = true;
        errdefer {
            if (free_on_incomplete) {
                switch (start_line) {
                    .request r => {
                        if (r.uri.len > 0) _.alloc.free(r.uri);
                        if (r.version.len > 0) _.alloc.free(r.version);
                    },
                    .response r => {
                        if (r.version.len > 0) _.alloc.free(r.version);
                        if (r.reason_phrase.len > 0) _.alloc.free(r.reason_phrase);
                    },
                }
            }
        }

        let headers_list: std.ArrayList(RtspHeader) = .empty;
        errdefer {
            if (free_on_incomplete) {
                for (h of headers_list.items) {
                    if (h.name.len > 0) _.alloc.free(h.name);
                    if (h.value.len > 0) _.alloc.free(h.value);
                }
                headers_list.deinit(_.alloc);
            }
        }

        let content_length: usize = 0;
        let saw_content_length = false;

        if (idx := first_crlf) {
            let cursor = idx + 2;
            while (cursor < header_block.len) {
                const next_opt = std.mem.indexOfPos(u8, header_block, cursor, "\r\n");
                const line_end = next_opt orelse header_block.len;
                const line = header_block[cursor..line_end];

                if (line.len > 0) {
                    const colon = std.mem.indexOfScalar(u8, line, ':') orelse return RtspError.MalformedHeader;
                    const hname = line[0..colon];
                    if (!isValidHeaderName(hname)) {
                        return RtspError.MalformedHeader;
                    }
                    const hval = trimAscii(line[colon + 1 ..]);

                    if (std.ascii.eqlIgnoreCase(hname, "Content-Length")) {
                        const parsed_cl = parseDecimalUsize(hval)?;
                        if (saw_content_length && parsed_cl != content_length) {
                            return RtspError.InvalidContentLength;
                        }
                        content_length = parsed_cl;
                        saw_content_length = true;
                        if (content_length > _.max_payload_bytes) {
                            return RtspError.PayloadTooLarge;
                        }
                    }

                    const name_dup = _.alloc.dupe(u8, hname) catch return RtspError.OutOfMemory;
                    errdefer _.alloc.free(name_dup);
                    const val_dup = if (hval.len > 0)
                        (_.alloc.dupe(u8, hval) catch return RtspError.OutOfMemory)
                    else
                        "";
                    headers_list.append(_.alloc, RtspHeader{
                        name: name_dup,
                        value: val_dup,
                    }) catch return RtspError.OutOfMemory;
                }

                cursor = if (next_opt != null) line_end + 2 else header_block.len;
            }
        }

        const total_needed = hdr_end + 4 + content_length;
        if (rem.len < total_needed) {
            // Incomplete body: free temporary allocations and wait for more bytes.
            switch (start_line) {
                .request r => {
                    if (r.uri.len > 0) _.alloc.free(r.uri);
                    if (r.version.len > 0) _.alloc.free(r.version);
                },
                .response r => {
                    if (r.version.len > 0) _.alloc.free(r.version);
                    if (r.reason_phrase.len > 0) _.alloc.free(r.reason_phrase);
                },
            }
            for (h of headers_list.items) {
                if (h.name.len > 0) _.alloc.free(h.name);
                if (h.value.len > 0) _.alloc.free(h.value);
            }
            headers_list.deinit(_.alloc);
            return null;
        }

        const body_dup = if (content_length > 0)
            (_.alloc.dupe(u8, rem[hdr_end + 4 .. total_needed]) catch return RtspError.OutOfMemory)
        else
            "";
        errdefer {
            if (body_dup.len > 0) _.alloc.free(body_dup);
        }

        const owned_headers = headers_list.toOwnedSlice(_.alloc) catch return RtspError.OutOfMemory;
        free_on_incomplete = false;
        _.consumeBytes(total_needed);

        return RtspStreamItem{
            message: RtspMessage{
                start_line: start_line,
                headers: owned_headers,
                body: body_dup,
            },
        };
    }
}

/// Fluent builder for RTSP/1.0 requests and responses.
/// Automatically increments `CSeq`, maintains `Content-Length`, and blocks CR/LF header injection.
pub struct RtspBuilder {
    alloc: std.mem.Allocator,
    next_cseq: u32,
    explicit_cseq: u32?,
    is_request: bool,
    has_target: bool,
    method: RtspMethod,
    uri: []const u8,
    status_code: u16,
    reason_phrase: []const u8,
    headers: std.ArrayList(RtspHeader),
    body: []const u8,

    /// Initializes a new `RtspBuilder` starting at `CSeq = 1`.
    pub static fn init(allocator: std.mem.Allocator): RtspBuilder {
        return RtspBuilder{
            alloc: allocator,
            next_cseq: 1,
            explicit_cseq: null,
            is_request: true,
            has_target: false,
            method: RtspMethod.Options,
            uri: "",
            status_code: 200,
            reason_phrase: "",
            headers: .empty,
            body: "",
        };
    }

    fn resetPending(*_) {
        if (_.uri.len > 0) {
            _.alloc.free(_.uri);
            _.uri = "";
        }
        if (_.reason_phrase.len > 0) {
            _.alloc.free(_.reason_phrase);
            _.reason_phrase = "";
        }
        for (h of _.headers.items) {
            if (h.name.len > 0) _.alloc.free(h.name);
            if (h.value.len > 0) _.alloc.free(h.value);
        }
        _.headers.clearRetainingCapacity();
        if (_.body.len > 0) {
            _.alloc.free(_.body);
            _.body = "";
        }
        _.explicit_cseq = null;
        _.has_target = false;
    }

    /// Frees any unbuilt pending allocations and internal header storage.
    pub fn deinit(*_) {
        _.resetPending();
        _.headers.deinit(_.alloc);
    }

    /// Starts building an RTSP/1.0 request with `method` and `uri`.
    pub fn request(*_, method: RtspMethod, uri: []const u8): (*RtspBuilder) ! RtspError {
        if (uri.len == 0 || containsCrOrLf(uri)) {
            return RtspError.HeaderInjection;
        }
        _.resetPending();
        _.uri = _.alloc.dupe(u8, uri) catch return RtspError.OutOfMemory;
        _.method = method;
        _.is_request = true;
        _.has_target = true;
        return _;
    }

    /// Starts building an RTSP/1.0 response with `status_code` and `reason_phrase`.
    pub fn response(*_, status_code: u16, reason_phrase: []const u8): (*RtspBuilder) ! RtspError {
        if (containsCrOrLf(reason_phrase)) {
            return RtspError.HeaderInjection;
        }
        if (status_code < 100 || status_code > 599) {
            return RtspError.InvalidStartLine;
        }
        _.resetPending();
        _.reason_phrase = if (reason_phrase.len > 0)
            (_.alloc.dupe(u8, reason_phrase) catch return RtspError.OutOfMemory)
        else
            "";
        _.status_code = status_code;
        _.is_request = false;
        _.has_target = true;
        return _;
    }

    /// Overrides the auto-incremented `CSeq` for the current message.
    pub fn cseq(*_, seq: u32): *RtspBuilder {
        _.explicit_cseq = seq;
        return _;
    }

    /// Appends a header after validating against CR/LF header injection.
    pub fn header(*_, name: []const u8, value: []const u8): (*RtspBuilder) ! RtspError {
        if (containsCrOrLf(name) || containsCrOrLf(value)) {
            return RtspError.HeaderInjection;
        }
        if (!isValidHeaderName(name)) {
            return RtspError.MalformedHeader;
        }
        const name_dup = _.alloc.dupe(u8, name) catch return RtspError.OutOfMemory;
        errdefer _.alloc.free(name_dup);
        const val_dup = if (value.len > 0)
            (_.alloc.dupe(u8, value) catch return RtspError.OutOfMemory)
        else
            "";
        _.headers.append(_.alloc, RtspHeader{
            name: name_dup,
            value: val_dup,
        }) catch return RtspError.OutOfMemory;
        return _;
    }

    /// Convenience helper to add a `Session` header.
    pub fn session(*_, session_id: []const u8): (*RtspBuilder) ! RtspError {
        return _.header("Session", session_id);
    }

    /// Convenience helper to format and add a `Transport` header.
    pub fn transport(*_, spec: TransportSpec): (*RtspBuilder) ! RtspError {
        const formatted = formatTransportHeader(_.alloc, spec)?;
        defer _.alloc.free(formatted);
        return _.header("Transport", formatted);
    }

    /// Sets the message body (e.g., SDP payload). `Content-Length` is maintained automatically.
    pub fn setBody(*_, body: []const u8): (*RtspBuilder) ! RtspError {
        if (_.body.len > 0) {
            _.alloc.free(_.body);
            _.body = "";
        }
        if (body.len > 0) {
            _.body = _.alloc.dupe(u8, body) catch return RtspError.OutOfMemory;
        }
        return _;
    }

    /// Finalizes and returns an owned `RtspMessage`, incrementing `CSeq` if auto-assigned.
    /// Caller owns the returned `RtspMessage` and must call `msg.deinit(allocator)`.
    pub fn build(*_): RtspMessage ! RtspError {
        if (!_.has_target) return RtspError.InvalidStartLine;

        let has_cseq = false;
        let has_content_length = false;
        for (h of _.headers.items) {
            if (std.ascii.eqlIgnoreCase(h.name, "CSeq")) has_cseq = true;
            if (std.ascii.eqlIgnoreCase(h.name, "Content-Length")) has_content_length = true;
        }

        if (!has_cseq) {
            let seq = _.next_cseq;
            if (exp := _.explicit_cseq) {
                seq = exp;
            } else {
                _.next_cseq += 1;
            }
            let seq_buf: [16]u8 = undefined;
            const seq_str = std.fmt.bufPrint(&seq_buf, "{d}", .{seq}) catch return RtspError.OutOfMemory;
            _.header("CSeq", seq_str)?;
        }

        if (_.body.len > 0 && !has_content_length) {
            let len_buf: [32]u8 = undefined;
            const len_str = std.fmt.bufPrint(&len_buf, "{d}", .{_.body.len}) catch return RtspError.OutOfMemory;
            _.header("Content-Length", len_str)?;
        }

        const ver_dup = _.alloc.dupe(u8, "RTSP/1.0") catch return RtspError.OutOfMemory;
        errdefer _.alloc.free(ver_dup);

        const start_line = if (_.is_request)
            RtspStartLine{
                request: RtspRequestLine{
                    method: _.method,
                    uri: _.uri,
                    version: ver_dup,
                },
            }
        else
            RtspStartLine{
                response: RtspResponseLine{
                    version: ver_dup,
                    status_code: _.status_code,
                    reason_phrase: _.reason_phrase,
                },
            };

        const owned_headers = _.headers.toOwnedSlice(_.alloc) catch return RtspError.OutOfMemory;
        const owned_body = _.body;

        // Transfer ownership to RtspMessage
        _.uri = "";
        _.reason_phrase = "";
        _.body = "";
        _.explicit_cseq = null;
        _.has_target = false;

        return RtspMessage{
            start_line: start_line,
            headers: owned_headers,
            body: owned_body,
        };
    }

    /// Finalizes the message and directly formats it into wire-protocol bytes.
    /// Caller owns the returned slice and must free it with `allocator.free(...)`.
    pub fn buildBytes(*_): []u8 ! RtspError {
        let msg = _.build()?;
        defer msg.deinit(_.alloc);
        return msg.format(_.alloc);
    }
}
