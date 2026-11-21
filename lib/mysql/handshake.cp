// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/HandshakeResponse41.cp | Synced: 2026-11-21T16:59:39Z
//! MySQL Protocol 4.1 `HandshakeResponse41` wire packet encoder & decoder
//! with capability flags, auth plugin response framing, connection attributes,
//! and deterministic `~init()` RAII buffer management.

import std from "std";

export const DEFAULT_MAX_PACKET_SIZE: u32 = 0xFFFFFF;
export const DEFAULT_CHARSET_UTF8MB4: u8 = 45;

// Key MySQL client capability flags used in HandshakeResponse41 framing
export const CAP_LONG_PASSWORD: u32 = 0x00000001;
export const CAP_CONNECT_WITH_DB: u32 = 0x00000008;
export const CAP_PROTOCOL_41: u32 = 0x00000200;
export const CAP_SECURE_CONNECTION: u32 = 0x00008000;
export const CAP_PLUGIN_AUTH: u32 = 0x00080000;
export const CAP_CONNECT_ATTRS: u32 = 0x00100000;
export const CAP_PLUGIN_AUTH_LENENC: u32 = 0x00200000;

export err HandshakeError {
    ShortPacket,
    InvalidHeader,
    InvalidLength,
    MissingNullTerminator,
    OutOfMemory,
}

export struct ConnectAttr {
    key: []const u8,
    value: []const u8,
}

export struct HandshakeResponse41 {
    capability_flags: u32 = CAP_LONG_PASSWORD | CAP_PROTOCOL_41 | CAP_SECURE_CONNECTION | CAP_PLUGIN_AUTH,
    max_packet_size: u32 = DEFAULT_MAX_PACKET_SIZE,
    character_set: u8 = DEFAULT_CHARSET_UTF8MB4,
    username: []const u8 = "",
    auth_response: []const u8 = "",
    database: []const u8 = "",
    auth_plugin_name: []const u8 = "caching_sha2_password",
    connect_attrs: []const ConnectAttr = &[],
    sequence_id: u8 = 1,
}

/// Owned encoded packet buffer with deterministic `~init()` RAII cleanup.
export struct EncodedHandshake {
    allocator: std.mem.Allocator,
    data: []u8,

    ~init() {
        if (_.data.len > 0) {
            _.allocator.free(_.data);
            _.data = &[];
        }
    }

    pub fn slice(): []const u8 {
        return _.data;
    }
}

fn lenEncIntSize(v: u64): usize {
    if (v < 0xFB) return 1;
    if (v <= 0xFFFF) return 3;
    if (v <= 0xFFFFFF) return 4;
    return 9;
}

fn appendU32Le(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u32): void! {
    const b: [4]u8 = [
        @truncate(v),
        @truncate(v >> 8),
        @truncate(v >> 16),
        @truncate(v >> 24),
    ];
    buf.appendSlice(alloc, b[0..])?;
}

fn appendLenEncInt(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u64): void! {
    if (v < 0xFB) {
        buf.append(alloc, @truncate(v))?;
    } else if (v <= 0xFFFF) {
        const b: [3]u8 = [0xFC, @truncate(v), @truncate(v >> 8)];
        buf.appendSlice(alloc, b[0..])?;
    } else if (v <= 0xFFFFFF) {
        const b: [4]u8 = [0xFD, @truncate(v), @truncate(v >> 8), @truncate(v >> 16)];
        buf.appendSlice(alloc, b[0..])?;
    } else {
        const b: [9]u8 = [
            0xFE,
            @truncate(v),
            @truncate(v >> 8),
            @truncate(v >> 16),
            @truncate(v >> 24),
            @truncate(v >> 32),
            @truncate(v >> 40),
            @truncate(v >> 48),
            @truncate(v >> 56),
        ];
        buf.appendSlice(alloc, b[0..])?;
    }
}

fn appendLenEncStr(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): void! {
    appendLenEncInt(*buf, alloc, @as(u64, s.len))?;
    buf.appendSlice(alloc, s)?;
}

fn appendCString(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): void! {
    buf.appendSlice(alloc, s)?;
    buf.append(alloc, 0)?;
}

/// Encode a `HandshakeResponse41` into a framed MySQL packet (4-byte header + payload).
export fn encodeHandshakeResponse41(allocator: std.mem.Allocator, req: HandshakeResponse41): EncodedHandshake! {
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(allocator);

    // 4-byte MySQL packet header placeholder
    const zeros4: [4]u8 = [0, 0, 0, 0];
    buf.appendSlice(allocator, zeros4[0..])?;

    let caps = req.capability_flags;
    if (req.database.len > 0) {
        caps |= CAP_CONNECT_WITH_DB;
    } else {
        caps &= ~CAP_CONNECT_WITH_DB;
    }
    if (req.connect_attrs.len > 0) {
        caps |= CAP_CONNECT_ATTRS;
    } else {
        caps &= ~CAP_CONNECT_ATTRS;
    }

    appendU32Le(*buf, allocator, caps)?;
    appendU32Le(*buf, allocator, req.max_packet_size)?;
    buf.append(allocator, req.character_set)?;

    // 23-byte reserved filler
    const filler: [23]u8 = [0; 23];
    buf.appendSlice(allocator, filler[0..])?;

    // username (NUL-terminated)
    appendCString(*buf, allocator, req.username)?;

    // auth_response framing depends on negotiated capabilities
    if ((caps & CAP_PLUGIN_AUTH_LENENC) != 0) {
        appendLenEncStr(*buf, allocator, req.auth_response)?;
    } else if ((caps & CAP_SECURE_CONNECTION) != 0) {
        if (req.auth_response.len > 255) return err.InvalidLength;
        buf.append(allocator, @truncate(req.auth_response.len))?;
        buf.appendSlice(allocator, req.auth_response)?;
    } else {
        appendCString(*buf, allocator, req.auth_response)?;
    }

    if ((caps & CAP_CONNECT_WITH_DB) != 0) {
        appendCString(*buf, allocator, req.database)?;
    }

    if ((caps & CAP_PLUGIN_AUTH) != 0) {
        appendCString(*buf, allocator, req.auth_plugin_name)?;
    }

    if ((caps & CAP_CONNECT_ATTRS) != 0) {
        let total_attr_bytes: usize = 0;
        let i: usize = 0;
        while (i < req.connect_attrs.len) : (i += 1) {
            const attr = req.connect_attrs[i];
            total_attr_bytes += lenEncIntSize(@as(u64, attr.key.len)) + attr.key.len;
            total_attr_bytes += lenEncIntSize(@as(u64, attr.value.len)) + attr.value.len;
        }
        appendLenEncInt(*buf, allocator, @as(u64, total_attr_bytes))?;
        let j: usize = 0;
        while (j < req.connect_attrs.len) : (j += 1) {
            appendLenEncStr(*buf, allocator, req.connect_attrs[j].key)?;
            appendLenEncStr(*buf, allocator, req.connect_attrs[j].value)?;
        }
    }

    const payload_len = buf.items.len - 4;
    if (payload_len > 0xFFFFFF) return err.InvalidLength;
    buf.items[0] = @truncate(payload_len);
    buf.items[1] = @truncate(payload_len >> 8);
    buf.items[2] = @truncate(payload_len >> 16);
    buf.items[3] = req.sequence_id;

    const owned = buf.toOwnedSlice(allocator)?;
    return EncodedHandshake{ allocator: allocator, data: owned };
}

export struct DecodedHandshake41 {
    sequence_id: u8,
    capability_flags: u32,
    max_packet_size: u32,
    character_set: u8,
    username: []const u8,
    auth_response: []const u8,
    database: []const u8,
    auth_plugin_name: []const u8,
    attrs_raw: []const u8,
}

fn readU32Le(b: []const u8, off: usize): u32! {
    if (off + 4 > b.len) return err.ShortPacket;
    return @as(u32, b[off])
        | (@as(u32, b[off + 1]) << 8)
        | (@as(u32, b[off + 2]) << 16)
        | (@as(u32, b[off + 3]) << 24);
}

struct CStringSlice {
    str: []const u8,
    next: usize,
}

fn readCStringAt(b: []const u8, pos_in: usize): CStringSlice! {
    let p = pos_in;
    while (p < b.len) : (p += 1) {
        if (b[p] == 0) {
            return CStringSlice{ str: b[pos_in..p], next: p + 1 };
        }
    }
    return err.MissingNullTerminator;
}

/// Decode a framed MySQL `HandshakeResponse41` packet (zero heap allocation, slices reference `packet`).
export fn decodeHandshakeResponse41(packet: []const u8): DecodedHandshake41! {
    if (packet.len < 4 + 32) return err.ShortPacket;
    const payload_len: usize = @as(usize, packet[0])
        | (@as(usize, packet[1]) << 8)
        | (@as(usize, packet[2]) << 16);
    const seq = packet[3];
    if (4 + payload_len > packet.len || payload_len < 32) return err.InvalidHeader;

    const body = packet[4 .. 4 + payload_len];
    const caps = readU32Le(body, 0)?;
    const max_pkt = readU32Le(body, 4)?;
    const charset = body[8];
    let pos: usize = 32; // 4 + 4 + 1 + 23 filler

    const u_res = readCStringAt(body, pos)?;
    const username = u_res.str;
    pos = u_res.next;

    let auth_response: []const u8 = "";
    if ((caps & CAP_PLUGIN_AUTH_LENENC) != 0) {
        if (pos >= body.len) return err.ShortPacket;
        const first = body[pos];
        if (first >= 0xFB) return err.InvalidLength;
        pos += 1;
        const n: usize = @as(usize, first);
        if (pos + n > body.len) return err.ShortPacket;
        auth_response = body[pos .. pos + n];
        pos += n;
    } else if ((caps & CAP_SECURE_CONNECTION) != 0) {
        if (pos >= body.len) return err.ShortPacket;
        const n: usize = @as(usize, body[pos]);
        pos += 1;
        if (pos + n > body.len) return err.ShortPacket;
        auth_response = body[pos .. pos + n];
        pos += n;
    } else {
        const a_res = readCStringAt(body, pos)?;
        auth_response = a_res.str;
        pos = a_res.next;
    }

    let database: []const u8 = "";
    if ((caps & CAP_CONNECT_WITH_DB) != 0) {
        const d_res = readCStringAt(body, pos)?;
        database = d_res.str;
        pos = d_res.next;
    }

    let plugin: []const u8 = "";
    if ((caps & CAP_PLUGIN_AUTH) != 0 && pos < body.len) {
        const p_res = readCStringAt(body, pos)?;
        plugin = p_res.str;
        pos = p_res.next;
    }

    let attrs_raw: []const u8 = "";
    if ((caps & CAP_CONNECT_ATTRS) != 0 && pos < body.len) {
        attrs_raw = body[pos..];
    }

    return DecodedHandshake41{
        sequence_id: seq,
        capability_flags: caps,
        max_packet_size: max_pkt,
        character_set: charset,
        username: username,
        auth_response: auth_response,
        database: database,
        auth_plugin_name: plugin,
        attrs_raw: attrs_raw,
    };
}
