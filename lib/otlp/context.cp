// Source: cp/lib/otlp/context.cp (W3C TraceContext & Baggage propagation)
const std = @import("std");

pub const MAX_BAGGAGE_BYTES: usize = 8192;
pub const MAX_BAGGAGE_ENTRIES: usize = 180;

pub const OtlpError = error{
    InvalidTraceParent,
    InvalidVersion,
    ZeroTraceId,
    ZeroSpanId,
    InvalidHex,
    InvalidBaggage,
    BaggageLimitExceeded,
    OutOfMemory,
};

pub struct TraceParent {
    version: u8,
    trace_id: [16]u8,
    span_id: [8]u8,
    trace_flags: u8,

    pub fn isSampled(_: *const TraceParent): bool {
        return (self.trace_flags & @as(u8, 0x01)) != 0;
    }

    pub fn newChildSpan(_: *const TraceParent, child_span_id: [8]u8): OtlpError!TraceParent {
        if (isAllZero(&child_span_id)) {
            return OtlpError.ZeroSpanId;
        }
        return TraceParent{
            .version = self.version,
            .trace_id = self.trace_id,
            .span_id = child_span_id,
            .trace_flags = self.trace_flags,
        };
    }
}

pub struct BaggageEntry {
    key: []const u8,
    value: []const u8,
    metadata: ([]const u8)?,
}

fn isAllZero(bytes: []const u8): bool {
    for (b of bytes) {
        if (b != 0) {
            return false;
        }
    }
    return true;
}

fn hexNibble(c: u8): OtlpError!u8 {
    if (c >= '0' and c <= '9') {
        return c - '0';
    }
    if (c >= 'a' and c <= 'f') {
        return c - 'a' + 10;
    }
    return OtlpError.InvalidHex;
}

fn parseHexBytes(hex: []const u8, out: []u8): OtlpError!void {
    if (hex.len != out.len * 2) {
        return OtlpError.InvalidTraceParent;
    }
    let i: usize = 0;
    while (i < out.len) : (i += 1) {
        const hi = try hexNibble(hex[i * 2]);
        const lo = try hexNibble(hex[i * 2 + 1]);
        out[i] = (hi << 4) | lo;
    }
}

pub fn writeHexLower(out: []u8, bytes: []const u8)  {
    const digits = "0123456789abcdef";
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        const b = bytes[i];
        out[i * 2] = digits[b >> 4];
        out[i * 2 + 1] = digits[b & @as(u8, 0x0f)];
    }
}

pub fn parseTraceParent(header: []const u8): OtlpError!TraceParent {
    if (header.len != 55) {
        return OtlpError.InvalidTraceParent;
    }
    if (header[2] != '-' or header[35] != '-' or header[52] != '-') {
        return OtlpError.InvalidTraceParent;
    }

    let ver_buf: [1]u8 = undefined;
    try parseHexBytes(header[0..2], &ver_buf);
    const version = ver_buf[0];
    if (version == @as(u8, 0xff) or version != 0) {
        return OtlpError.InvalidVersion;
    }

    let trace_id: [16]u8 = undefined;
    try parseHexBytes(header[3..35], &trace_id);
    if (isAllZero(&trace_id)) {
        return OtlpError.ZeroTraceId;
    }

    let span_id: [8]u8 = undefined;
    try parseHexBytes(header[36..52], &span_id);
    if (isAllZero(&span_id)) {
        return OtlpError.ZeroSpanId;
    }

    let flags_buf: [1]u8 = undefined;
    try parseHexBytes(header[53..55], &flags_buf);

    return TraceParent{
        .version = version,
        .trace_id = trace_id,
        .span_id = span_id,
        .trace_flags = flags_buf[0],
    };
}

pub fn formatTraceParent(alloc: std.mem.Allocator, tp: *const TraceParent): OtlpError![]u8 {
    let buf = alloc.alloc(u8, 55) catch return OtlpError.OutOfMemory;
    const digits = "0123456789abcdef";
    buf[0] = digits[tp.version >> 4];
    buf[1] = digits[tp.version & @as(u8, 0x0f)];
    buf[2] = '-';
    writeHexLower(buf[3..35], &tp.trace_id);
    buf[35] = '-';
    writeHexLower(buf[36..52], &tp.span_id);
    buf[52] = '-';
    buf[53] = digits[tp.trace_flags >> 4];
    buf[54] = digits[tp.trace_flags & @as(u8, 0x0f)];
    return buf;
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len and (s[start] == ' ' or s[start] == '\t')) : (start += 1) {}
    let end: usize = s.len;
    while (end > start and (s[end - 1] == ' ' or s[end - 1] == '\t')) : (end -= 1) {}
    return s[start..end];
}

fn hexAnyCaseNibble(c: u8): OtlpError!u8 {
    if (c >= '0' and c <= '9') {
        return c - '0';
    }
    if (c >= 'a' and c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' and c <= 'F') {
        return c - 'A' + 10;
    }
    return OtlpError.InvalidBaggage;
}

fn percentDecode(alloc: std.mem.Allocator, input: []const u8): OtlpError![]u8 {
    let list = std.ArrayList(u8).empty;
    errdefer list.deinit(alloc);

    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == '%') {
            if (i + 2 >= input.len) {
                return OtlpError.InvalidBaggage;
            }
            const hi = try hexAnyCaseNibble(input[i + 1]);
            const lo = try hexAnyCaseNibble(input[i + 2]);
            list.append(alloc, (hi << 4) | lo) catch return OtlpError.OutOfMemory;
            i += 3;
        } else {
            list.append(alloc, input[i]) catch return OtlpError.OutOfMemory;
            i += 1;
        }
    }
    return list.toOwnedSlice(alloc) catch return OtlpError.OutOfMemory;
}

fn isUnreservedBaggageChar(c: u8): bool {
    if ((c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or (c >= '0' and c <= '9')) {
        return true;
    }
    return c == '-' or c == '.' or c == '_' or c == '~';
}

fn appendPercentEncoded(alloc: std.mem.Allocator, list: *std.ArrayList(u8), input: []const u8): OtlpError!void {
    const digits = "0123456789ABCDEF";
    for (c of input) {
        if (isUnreservedBaggageChar(c)) {
            list.append(alloc, c) catch return OtlpError.OutOfMemory;
        } else {
            list.append(alloc, '%') catch return OtlpError.OutOfMemory;
            list.append(alloc, digits[c >> 4]) catch return OtlpError.OutOfMemory;
            list.append(alloc, digits[c & @as(u8, 0x0f)]) catch return OtlpError.OutOfMemory;
        }
    }
}

pub fn freeBaggage(alloc: std.mem.Allocator, entries: []BaggageEntry)  {
    for (entry of entries) {
        alloc.free(entry.key);
        alloc.free(entry.value);
        if (entry.metadata != null) {
            alloc.free(entry.metadata.?);
        }
    }
    alloc.free(entries);
}

pub fn parseBaggage(alloc: std.mem.Allocator, header: []const u8): OtlpError![]BaggageEntry {
    if (header.len > MAX_BAGGAGE_BYTES) {
        return OtlpError.BaggageLimitExceeded;
    }

    let list = std.ArrayList(BaggageEntry).empty;
    errdefer {
        for (entry of list.items) {
            alloc.free(entry.key);
            alloc.free(entry.value);
            if (entry.metadata != null) {
                alloc.free(entry.metadata.?);
            }
        }
        list.deinit(alloc);
    }

    const trimmed_header = trimAscii(header);
    if (trimmed_header.len == 0) {
        return list.toOwnedSlice(alloc) catch return OtlpError.OutOfMemory;
    }

    let it = std.mem.splitScalar(u8, trimmed_header, ',');
    while (raw_member := it.next()) {
        const member = trimAscii(raw_member);
        if (member.len == 0) {
            return OtlpError.InvalidBaggage;
        }
        if (list.items.len >= MAX_BAGGAGE_ENTRIES) {
            return OtlpError.BaggageLimitExceeded;
        }

        let kv_part = member;
        let meta_part: ([]const u8)? = null;
        if (semi_idx := std.mem.indexOfScalar(u8, member, ';')) {
            kv_part = trimAscii(member[0..semi_idx]);
            const raw_meta = trimAscii(member[semi_idx + 1 ..]);
            if (raw_meta.len > 0) {
                meta_part = raw_meta;
            }
        }

        const eq_idx = std.mem.indexOfScalar(u8, kv_part, '=') orelse return OtlpError.InvalidBaggage;
        const raw_key = trimAscii(kv_part[0..eq_idx]);
        const raw_val = trimAscii(kv_part[eq_idx + 1 ..]);
        if (raw_key.len == 0) {
            return OtlpError.InvalidBaggage;
        }

        const key = try percentDecode(alloc, raw_key);
        errdefer alloc.free(key);
        const value = try percentDecode(alloc, raw_val);
        errdefer alloc.free(value);

        let metadata_copy: ([]const u8)? = null;
        if (m := meta_part) {
            metadata_copy = alloc.dupe(u8, m) catch return OtlpError.OutOfMemory;
        }

        list.append(alloc, BaggageEntry{
            .key = key,
            .value = value,
            .metadata = metadata_copy,
        }) catch return OtlpError.OutOfMemory;
    }

    return list.toOwnedSlice(alloc) catch return OtlpError.OutOfMemory;
}

pub fn formatBaggage(alloc: std.mem.Allocator, entries: []const BaggageEntry): OtlpError![]u8 {
    if (entries.len > MAX_BAGGAGE_ENTRIES) {
        return OtlpError.BaggageLimitExceeded;
    }

    let list = std.ArrayList(u8).empty;
    errdefer list.deinit(alloc);

    let i: usize = 0;
    while (i < entries.len) : (i += 1) {
        if (i > 0) {
            list.append(alloc, ',') catch return OtlpError.OutOfMemory;
        }
        const entry = entries[i];
        try appendPercentEncoded(alloc, &list, entry.key);
        list.append(alloc, '=') catch return OtlpError.OutOfMemory;
        try appendPercentEncoded(alloc, &list, entry.value);
        if (entry.metadata != null) {
            list.append(alloc, ';') catch return OtlpError.OutOfMemory;
            list.appendSlice(alloc, entry.metadata.?) catch return OtlpError.OutOfMemory;
        }
    }

    if (list.items.len > MAX_BAGGAGE_BYTES) {
        return OtlpError.BaggageLimitExceeded;
    }

    return list.toOwnedSlice(alloc) catch return OtlpError.OutOfMemory;
}
