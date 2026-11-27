// Source: cp/lib/nats/nats.cp (NATS Wire Protocol, HPUB/HMSG Headers & Incremental Stream Parser)
// Pure CP implementation of NATS client/server wire protocol encoder and incremental
// chunk-boundary-agnostic stream parser with NATS/1.0 header support and security guards.

const std = @import("std");

pub const DEFAULT_MAX_PAYLOAD_BYTES: usize = 1048576; // 1 MiB
pub const NATS_HEADER_PREFIX: []const u8 = "NATS/1.0";

pub err NatsError {
    InvalidProtocolLine,
    InvalidSubject,
    InvalidPayloadSize,
    PayloadTooLarge,
    InvalidNatsHeaders,
    CommandInjection,
    OutOfMemory,
}

pub struct NatsHeader {
    name: []const u8 = "",
    value: []const u8 = "",

    pub static fn init(name: []const u8, value: []const u8): NatsHeader {
        return NatsHeader{ name: name, value: value };
    }
}

const empty_headers_storage: [0]NatsHeader = [];

pub struct NatsMsg {
    subject: []const u8 = "",
    sid: []const u8 = "",
    reply_to: ([]const u8)? = null,
    headers: []const NatsHeader = &empty_headers_storage,
    payload: []const u8 = "",

    pub fn deinit(alloc: std.mem.Allocator) {
        if (_.subject.len > 0) {
            alloc.free(_.subject);
        }
        if (_.sid.len > 0) {
            alloc.free(_.sid);
        }
        if (rt := _.reply_to) {
            if (rt.len > 0) {
                alloc.free(rt);
            }
        }
        let i: usize = 0;
        while (i < _.headers.len) : (i += 1) {
            if (_.headers[i].name.len > 0) {
                alloc.free(_.headers[i].name);
            }
            if (_.headers[i].value.len > 0) {
                alloc.free(_.headers[i].value);
            }
        }
        if (_.headers.len > 0) {
            alloc.free(_.headers);
        }
        if (_.payload.len > 0) {
            alloc.free(_.payload);
        }
    }

    pub fn header(name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.headers.len) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(_.headers[i].name, name)) {
                return _.headers[i].value;
            }
        }
        return null;
    }
}

pub enum NatsOp {
    info: []const u8,
    msg: NatsMsg,
    ping,
    pong,
    ok,
    err: []const u8,

    pub fn deinit(alloc: std.mem.Allocator) {
        switch (_) {
            .info s => {
                if (s.len > 0) alloc.free(s);
            },
            .msg m => {
                m.deinit(alloc);
            },
            .err s => {
                if (s.len > 0) alloc.free(s);
            },
            else => {},
        }
    }
}

// ============================================================================
// Validation & Parsing Helpers
// ============================================================================

fn hasCrLf(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '\r' || s[i] == '\n') {
            return true;
        }
    }
    return false;
}

fn hasWhitespace(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == ' ' || s[i] == '\t') {
            return true;
        }
    }
    return false;
}

fn validateSubject(subject: []const u8): void! {
    if (hasCrLf(subject)) return err.CommandInjection;
    if (subject.len == 0 || hasWhitespace(subject)) return err.InvalidSubject;
}

fn validateToken(token: []const u8): void! {
    if (hasCrLf(token)) return err.CommandInjection;
    if (token.len == 0 || hasWhitespace(token)) return err.InvalidSubject;
}

fn validateHeader(h: NatsHeader): void! {
    if (hasCrLf(h.name) || hasCrLf(h.value)) return err.CommandInjection;
    if (h.name.len == 0 || hasWhitespace(h.name)) return err.InvalidNatsHeaders;
    let i: usize = 0;
    while (i < h.name.len) : (i += 1) {
        if (h.name[i] == ':') return err.InvalidNatsHeaders;
    }
}

fn dupeBytes(alloc: std.mem.Allocator, src: []const u8): ([]u8)! {
    if (src.len == 0) return "";
    return alloc.dupe(u8, src) catch return err.OutOfMemory;
}

fn appendSlice(alloc: std.mem.Allocator, list: *std.ArrayListUnmanaged(u8), data: []const u8): void! {
    list.appendSlice(alloc, data) catch return err.OutOfMemory;
}

fn appendUsizeDecimal(alloc: std.mem.Allocator, list: *std.ArrayListUnmanaged(u8), val: usize): void! {
    let tmp: [32]u8 = undefined;
    let idx: usize = 32;
    let v = val;
    if (v == 0) {
        appendSlice(alloc, list, "0")?;
        return;
    }
    while (v > 0) {
        idx -= 1;
        tmp[idx] = ('0' as u8) + ((v % 10) as u8);
        v /= 10;
    }
    appendSlice(alloc, list, tmp[idx..32])?;
}

fn parseStrictUsize(s: []const u8): usize! {
    if (s.len == 0) return err.InvalidPayloadSize;
    let val: usize = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return err.InvalidPayloadSize;
        }
        const digit = (c - '0') as usize;
        if (val > (std.math.maxInt(usize) - digit) / 10) {
            return err.PayloadTooLarge;
        }
        val = val * 10 + digit;
    }
    return val;
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
    return s[start..end];
}

fn findCrLf(data: []const u8, start: usize): usize? {
    if (data.len < start + 2) return null;
    let i: usize = start;
    while (i + 1 < data.len) : (i += 1) {
        if (data[i] == '\r' && data[i + 1] == '\n') {
            return i;
        }
    }
    return null;
}

fn splitTokens(line: []const u8, out_tokens: *[8][]const u8): usize! {
    let count: usize = 0;
    let i: usize = 0;
    while (i < line.len) {
        while (i < line.len && (line[i] == ' ' || line[i] == '\t')) : (i += 1) {}
        if (i >= line.len) break;
        const start = i;
        while (i < line.len && line[i] != ' ' && line[i] != '\t') : (i += 1) {}
        if (count >= 8) return err.InvalidProtocolLine;
        out_tokens[count] = line[start..i];
        count += 1;
    }
    return count;
}

fn deinitHeadersSlice(alloc: std.mem.Allocator, list: *std.ArrayListUnmanaged(NatsHeader)) {
    let i: usize = 0;
    while (i < list.items.len) : (i += 1) {
        if (list.items[i].name.len > 0) alloc.free(list.items[i].name);
        if (list.items[i].value.len > 0) alloc.free(list.items[i].value);
    }
    list.deinit(alloc);
}

fn parseNatsHeaders(alloc: std.mem.Allocator, raw: []const u8): ([]const NatsHeader)! {
    if (raw.len == 0) {
        return &empty_headers_storage;
    }

    // Must start with "NATS/1.0" followed by CRLF or space (e.g., status line)
    const first_crlf = findCrLf(raw, 0) orelse return err.InvalidNatsHeaders;
    const version_line = raw[0..first_crlf];
    if (!std.mem.startsWith(u8, version_line, NATS_HEADER_PREFIX)) {
        return err.InvalidNatsHeaders;
    }
    if (version_line.len > NATS_HEADER_PREFIX.len && version_line[NATS_HEADER_PREFIX.len] != ' ') {
        return err.InvalidNatsHeaders;
    }

    let list: std.ArrayListUnmanaged(NatsHeader) = .{};
    errdefer deinitHeadersSlice(alloc, &list);

    let pos: usize = first_crlf + 2;
    let saw_terminator = false;
    while (pos < raw.len) {
        const line_end = findCrLf(raw, pos) orelse return err.InvalidNatsHeaders;
        const line = raw[pos..line_end];
        pos = line_end + 2;

        if (line.len == 0) {
            saw_terminator = true;
            break;
        }

        const colon_idx = std.mem.indexOfScalar(u8, line, ':') orelse return err.InvalidNatsHeaders;
        const raw_name = trimSpaces(line[0..colon_idx]);
        if (raw_name.len == 0 || hasWhitespace(raw_name)) {
            return err.InvalidNatsHeaders;
        }
        const raw_val = trimSpaces(line[(colon_idx + 1)..line.len]);

        const name_copy = dupeBytes(alloc, raw_name)?;
        errdefer if (name_copy.len > 0) alloc.free(name_copy);
        const val_copy = dupeBytes(alloc, raw_val)?;
        errdefer if (val_copy.len > 0) alloc.free(val_copy);

        list.append(alloc, NatsHeader{
            name: name_copy,
            value: val_copy,
        }) catch return err.OutOfMemory;
    }

    if (!saw_terminator || pos != raw.len) {
        return err.InvalidNatsHeaders;
    }

    if (list.items.len == 0) {
        list.deinit(alloc);
        return &empty_headers_storage;
    }

    return list.toOwnedSlice(alloc) catch return err.OutOfMemory;
}

// ============================================================================
// NatsEncoder (Client -> Server Command Framing)
// ============================================================================

pub struct NatsEncoder {
    pub static fn encodeConnect(alloc: std.mem.Allocator, json_opts: []const u8): ([]u8)! {
        if (hasCrLf(json_opts)) return err.CommandInjection;
        let out: std.ArrayListUnmanaged(u8) = .{};
        errdefer out.deinit(alloc);

        appendSlice(alloc, &out, "CONNECT ")?;
        appendSlice(alloc, &out, json_opts)?;
        appendSlice(alloc, &out, "\r\n")?;
        return out.toOwnedSlice(alloc) catch return err.OutOfMemory;
    }

    pub static fn encodePub(
        alloc: std.mem.Allocator,
        subject: []const u8,
        reply_to: ([]const u8)?,
        payload: []const u8,
    ): ([]u8)! {
        validateSubject(subject)?;
        if (rt := reply_to) {
            validateToken(rt)?;
        }

        let out: std.ArrayListUnmanaged(u8) = .{};
        errdefer out.deinit(alloc);

        appendSlice(alloc, &out, "PUB ")?;
        appendSlice(alloc, &out, subject)?;
        appendSlice(alloc, &out, " ")?;
        if (rt := reply_to) {
            appendSlice(alloc, &out, rt)?;
            appendSlice(alloc, &out, " ")?;
        }
        appendUsizeDecimal(alloc, &out, payload.len)?;
        appendSlice(alloc, &out, "\r\n")?;
        appendSlice(alloc, &out, payload)?;
        appendSlice(alloc, &out, "\r\n")?;
        return out.toOwnedSlice(alloc) catch return err.OutOfMemory;
    }

    pub static fn encodeHpub(
        alloc: std.mem.Allocator,
        subject: []const u8,
        reply_to: ([]const u8)?,
        headers: []const NatsHeader,
        payload: []const u8,
    ): ([]u8)! {
        validateSubject(subject)?;
        if (rt := reply_to) {
            validateToken(rt)?;
        }

        let hdr_buf: std.ArrayListUnmanaged(u8) = .{};
        defer hdr_buf.deinit(alloc);

        appendSlice(alloc, &hdr_buf, "NATS/1.0\r\n")?;
        let i: usize = 0;
        while (i < headers.len) : (i += 1) {
            validateHeader(headers[i])?;
            appendSlice(alloc, &hdr_buf, headers[i].name)?;
            appendSlice(alloc, &hdr_buf, ": ")?;
            appendSlice(alloc, &hdr_buf, headers[i].value)?;
            appendSlice(alloc, &hdr_buf, "\r\n")?;
        }
        appendSlice(alloc, &hdr_buf, "\r\n")?;

        const hdr_len = hdr_buf.items.len;
        const total_len = hdr_len + payload.len;

        let out: std.ArrayListUnmanaged(u8) = .{};
        errdefer out.deinit(alloc);

        appendSlice(alloc, &out, "HPUB ")?;
        appendSlice(alloc, &out, subject)?;
        appendSlice(alloc, &out, " ")?;
        if (rt := reply_to) {
            appendSlice(alloc, &out, rt)?;
            appendSlice(alloc, &out, " ")?;
        }
        appendUsizeDecimal(alloc, &out, hdr_len)?;
        appendSlice(alloc, &out, " ")?;
        appendUsizeDecimal(alloc, &out, total_len)?;
        appendSlice(alloc, &out, "\r\n")?;
        appendSlice(alloc, &out, hdr_buf.items)?;
        appendSlice(alloc, &out, payload)?;
        appendSlice(alloc, &out, "\r\n")?;
        return out.toOwnedSlice(alloc) catch return err.OutOfMemory;
    }

    pub static fn encodeSub(
        alloc: std.mem.Allocator,
        subject: []const u8,
        queue_group: ([]const u8)?,
        sid: []const u8,
    ): ([]u8)! {
        validateSubject(subject)?;
        if (qg := queue_group) {
            validateToken(qg)?;
        }
        validateToken(sid)?;

        let out: std.ArrayListUnmanaged(u8) = .{};
        errdefer out.deinit(alloc);

        appendSlice(alloc, &out, "SUB ")?;
        appendSlice(alloc, &out, subject)?;
        appendSlice(alloc, &out, " ")?;
        if (qg := queue_group) {
            appendSlice(alloc, &out, qg)?;
            appendSlice(alloc, &out, " ")?;
        }
        appendSlice(alloc, &out, sid)?;
        appendSlice(alloc, &out, "\r\n")?;
        return out.toOwnedSlice(alloc) catch return err.OutOfMemory;
    }

    pub static fn encodeUnsub(
        alloc: std.mem.Allocator,
        sid: []const u8,
        max_msgs: usize?,
    ): ([]u8)! {
        validateToken(sid)?;

        let out: std.ArrayListUnmanaged(u8) = .{};
        errdefer out.deinit(alloc);

        appendSlice(alloc, &out, "UNSUB ")?;
        appendSlice(alloc, &out, sid)?;
        if (mm := max_msgs) {
            appendSlice(alloc, &out, " ")?;
            appendUsizeDecimal(alloc, &out, mm)?;
        }
        appendSlice(alloc, &out, "\r\n")?;
        return out.toOwnedSlice(alloc) catch return err.OutOfMemory;
    }

    pub static fn encodePing(alloc: std.mem.Allocator): ([]u8)! {
        return dupeBytes(alloc, "PING\r\n");
    }

    pub static fn encodePong(alloc: std.mem.Allocator): ([]u8)! {
        return dupeBytes(alloc, "PONG\r\n");
    }
}

// ============================================================================
// NatsParser (Incremental Chunk-Boundary-Agnostic Stream Parser)
// ============================================================================

pub struct NatsParser {
    alloc: std.mem.Allocator,
    max_payload_bytes: usize = DEFAULT_MAX_PAYLOAD_BYTES,
    buf: std.ArrayListUnmanaged(u8) = .{},
    pos: usize = 0,

    pub static fn init(alloc: std.mem.Allocator, max_payload_bytes: usize): NatsParser {
        return NatsParser{
            alloc: alloc,
            max_payload_bytes: max_payload_bytes,
            buf: .{},
            pos: 0,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
        _.pos = 0;
    }

    pub fn bufferedLen(): usize {
        return _.buf.items.len - _.pos;
    }

    pub fn feed(*_, chunk: []const u8): void! {
        if (chunk.len == 0) return;
        _.buf.appendSlice(_.alloc, chunk) catch return err.OutOfMemory;
    }

    fn consumeBytes(*_, count: usize) {
        _.pos += count;
        if (_.pos >= _.buf.items.len) {
            _.buf.clearRetainingCapacity();
            _.pos = 0;
        } else if (_.pos > 4096 && _.pos * 2 >= _.buf.items.len) {
            const rem = _.buf.items.len - _.pos;
            std.mem.copyForwards(u8, _.buf.items[0..rem], _.buf.items[_.pos.._.buf.items.len]);
            _.buf.items.len = rem;
            _.pos = 0;
        }
    }

    pub fn nextOp(*_): (NatsOp?)! {
        const line_end = findCrLf(_.buf.items, _.pos) orelse return null;
        const line = _.buf.items[_.pos..line_end];
        const after_line = (line_end - _.pos) + 2;

        if (line.len == 0) {
            return err.InvalidProtocolLine;
        }

        // Fast exact control lines: PING, PONG, +OK
        if (std.ascii.eqlIgnoreCase(line, "PING")) {
            _.consumeBytes(after_line);
            return NatsOp.ping;
        }
        if (std.ascii.eqlIgnoreCase(line, "PONG")) {
            _.consumeBytes(after_line);
            return NatsOp.pong;
        }
        if (std.ascii.eqlIgnoreCase(line, "+OK")) {
            _.consumeBytes(after_line);
            return NatsOp.ok;
        }

        // INFO <json>
        if (line.len >= 4 && std.ascii.eqlIgnoreCase(line[0..4], "INFO")) {
            if (line.len == 4 || (line[4] != ' ' && line[4] != '\t')) {
                return err.InvalidProtocolLine;
            }
            const info_body = trimSpaces(line[4..line.len]);
            if (info_body.len == 0) {
                return err.InvalidProtocolLine;
            }
            const info_copy = dupeBytes(_.alloc, info_body)?;
            _.consumeBytes(after_line);
            return NatsOp{ info: info_copy };
        }

        // -ERR <message>
        if (line.len >= 4 && std.ascii.eqlIgnoreCase(line[0..4], "-ERR")) {
            if (line.len > 4 && line[4] != ' ' && line[4] != '\t') {
                return err.InvalidProtocolLine;
            }
            const err_body = if (line.len > 4) trimSpaces(line[4..line.len]) else "";
            const err_copy = dupeBytes(_.alloc, err_body)?;
            _.consumeBytes(after_line);
            return NatsOp{ err: err_copy };
        }

        // MSG <subject> <sid> [reply-to] <#bytes>
        if (line.len >= 3 && std.ascii.eqlIgnoreCase(line[0..3], "MSG") && (line.len == 3 || line[3] == ' ' || line[3] == '\t')) {
            let tokens: [8][]const u8 = undefined;
            const n = splitTokens(line, &tokens)?;
            if (n != 4 && n != 5) {
                return err.InvalidProtocolLine;
            }

            const subj_raw = tokens[1];
            const sid_raw = tokens[2];
            const reply_raw: ([]const u8)? = if (n == 5) tokens[3] else null;
            const size_raw = if (n == 5) tokens[4] else tokens[3];

            const payload_len = parseStrictUsize(size_raw)?;
            if (payload_len > _.max_payload_bytes) {
                return err.PayloadTooLarge;
            }

            const needed = after_line + payload_len + 2;
            if (_.bufferedLen() < needed) {
                return null;
            }

            const payload_start = _.pos + after_line;
            const payload_end = payload_start + payload_len;
            if (_.buf.items[payload_end] != '\r' || _.buf.items[payload_end + 1] != '\n') {
                return err.InvalidProtocolLine;
            }

            const subj_copy = dupeBytes(_.alloc, subj_raw)?;
            errdefer if (subj_copy.len > 0) _.alloc.free(subj_copy);

            const sid_copy = dupeBytes(_.alloc, sid_raw)?;
            errdefer if (sid_copy.len > 0) _.alloc.free(sid_copy);

            let reply_copy: ([]const u8)? = null;
            if (rt := reply_raw) {
                reply_copy = dupeBytes(_.alloc, rt)?;
            }
            errdefer {
                if (rc := reply_copy) {
                    if (rc.len > 0) _.alloc.free(rc);
                }
            }

            const payload_copy = dupeBytes(_.alloc, _.buf.items[payload_start..payload_end])?;
            _.consumeBytes(needed);

            return NatsOp{
                msg: NatsMsg{
                    subject: subj_copy,
                    sid: sid_copy,
                    reply_to: reply_copy,
                    headers: &empty_headers_storage,
                    payload: payload_copy,
                },
            };
        }

        // HMSG <subject> <sid> [reply-to] <#header-bytes> <#total-bytes>
        if (line.len >= 4 && std.ascii.eqlIgnoreCase(line[0..4], "HMSG") && (line.len == 4 || line[4] == ' ' || line[4] == '\t')) {
            let tokens: [8][]const u8 = undefined;
            const n = splitTokens(line, &tokens)?;
            if (n != 5 && n != 6) {
                return err.InvalidProtocolLine;
            }

            const subj_raw = tokens[1];
            const sid_raw = tokens[2];
            const reply_raw: ([]const u8)? = if (n == 6) tokens[3] else null;
            const hdr_size_raw = if (n == 6) tokens[4] else tokens[3];
            const total_size_raw = if (n == 6) tokens[5] else tokens[4];

            const hdr_len = parseStrictUsize(hdr_size_raw)?;
            const total_len = parseStrictUsize(total_size_raw)?;
            if (total_len > _.max_payload_bytes) {
                return err.PayloadTooLarge;
            }
            if (hdr_len > total_len) {
                return err.InvalidNatsHeaders;
            }

            const needed = after_line + total_len + 2;
            if (_.bufferedLen() < needed) {
                return null;
            }

            const block_start = _.pos + after_line;
            const hdr_end = block_start + hdr_len;
            const total_end = block_start + total_len;
            if (_.buf.items[total_end] != '\r' || _.buf.items[total_end + 1] != '\n') {
                return err.InvalidProtocolLine;
            }

            const parsed_headers = parseNatsHeaders(_.alloc, _.buf.items[block_start..hdr_end])?;
            errdefer {
                let hi: usize = 0;
                while (hi < parsed_headers.len) : (hi += 1) {
                    if (parsed_headers[hi].name.len > 0) _.alloc.free(parsed_headers[hi].name);
                    if (parsed_headers[hi].value.len > 0) _.alloc.free(parsed_headers[hi].value);
                }
                if (parsed_headers.len > 0) _.alloc.free(parsed_headers);
            }

            const subj_copy = dupeBytes(_.alloc, subj_raw)?;
            errdefer if (subj_copy.len > 0) _.alloc.free(subj_copy);

            const sid_copy = dupeBytes(_.alloc, sid_raw)?;
            errdefer if (sid_copy.len > 0) _.alloc.free(sid_copy);

            let reply_copy: ([]const u8)? = null;
            if (rt := reply_raw) {
                reply_copy = dupeBytes(_.alloc, rt)?;
            }
            errdefer {
                if (rc := reply_copy) {
                    if (rc.len > 0) _.alloc.free(rc);
                }
            }

            const payload_copy = dupeBytes(_.alloc, _.buf.items[hdr_end..total_end])?;
            _.consumeBytes(needed);

            return NatsOp{
                msg: NatsMsg{
                    subject: subj_copy,
                    sid: sid_copy,
                    reply_to: reply_copy,
                    headers: parsed_headers,
                    payload: payload_copy,
                },
            };
        }

        return err.InvalidProtocolLine;
    }
}
