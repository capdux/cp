// Source: cp/lib/dns_wire/codec.cp (Pure CP RFC 1035/6891 DNS Wire Codec & Compression)
const std = @import("std");
const types = @import("./types.cp");

const DNS_HEADER_LEN = types.DNS_HEADER_LEN;
const DNS_MAX_LABEL_LEN = types.DNS_MAX_LABEL_LEN;
const DNS_MAX_NAME_LEN = types.DNS_MAX_NAME_LEN;
const DNS_MAX_POINTER_JUMPS = types.DNS_MAX_POINTER_JUMPS;
const DnsWireError = types.DnsWireError;
const DnsRecordType = types.DnsRecordType;
const DnsHeader = types.DnsHeader;
const DnsQuestion = types.DnsQuestion;
const MxData = types.MxData;
const SrvData = types.SrvData;
const SoaData = types.SoaData;
const DnsRdata = types.DnsRdata;
const DnsResourceRecord = types.DnsResourceRecord;
const DnsPacket = types.DnsPacket;

type StrSlice = []const u8;

pub const DecodedName = struct {
    name: []const u8,
    next_offset: usize,
};

pub fn decodeDnsName(alloc: std.mem.Allocator, packet: []const u8, start_offset: usize): DnsWireError!DecodedName {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    let pos: usize = start_offset;
    let end_offset: usize = 0;
    let jumped: bool = false;
    let jumps: usize = 0;
    let total_wire_len: usize = 0;

    while (true) {
        if (pos >= packet.len) {
            return DnsWireError.UnexpectedEof;
        }
        const len_byte = packet[pos];
        const tag = len_byte & 0xC0;
        if (tag == 0xC0) {
            if (pos + 1 >= packet.len) {
                return DnsWireError.UnexpectedEof;
            }
            if (!jumped) {
                end_offset = pos + 2;
                jumped = true;
            }
            jumps += 1;
            if (jumps > DNS_MAX_POINTER_JUMPS) {
                return DnsWireError.CompressionLoop;
            }
            const ptr = (@as(usize, len_byte & 0x3F) << 8) | @as(usize, packet[pos + 1]);
            if (ptr >= packet.len or ptr < DNS_HEADER_LEN) {
                return DnsWireError.InvalidLabel;
            }
            pos = ptr;
            continue;
        }
        if (tag != 0) {
            return DnsWireError.InvalidLabel;
        }
        if (len_byte == 0) {
            if (!jumped) {
                end_offset = pos + 1;
            }
            break;
        }
        const label_len = @as(usize, len_byte);
        if (label_len > DNS_MAX_LABEL_LEN) {
            return DnsWireError.InvalidLabel;
        }
        if (pos + 1 + label_len > packet.len) {
            return DnsWireError.UnexpectedEof;
        }
        total_wire_len += 1 + label_len;
        if (total_wire_len + 1 > DNS_MAX_NAME_LEN) {
            return DnsWireError.NameTooLong;
        }
        if (out.items.len > 0) {
            try out.append(alloc, '.');
        }
        try out.appendSlice(alloc, packet[pos + 1 .. pos + 1 + label_len]);
        pos += 1 + label_len;
    }

    return DecodedName{
        .name = try out.toOwnedSlice(alloc),
        .next_offset = end_offset,
    };
}

const CompEntry = struct {
    suffix: []const u8,
    offset: u16,
};

fn findSuffixOffset(entries: []const CompEntry, suffix: []const u8): u16? {
    for (e of entries) {
        if (std.ascii.eqlIgnoreCase(e.suffix, suffix)) {
            return e.offset;
        }
    }
    return null;
}

pub fn encodeDnsName(
    buf: *std.ArrayList(u8),
    alloc: std.mem.Allocator,
    raw_name: []const u8,
    comp_entries: *std.ArrayList(CompEntry)?,
): DnsWireError!void {
    const trimmed = std.mem.trim(u8, raw_name, ".");
    if (trimmed.len == 0) {
        try buf.append(alloc, 0);
        return;
    }
    if (trimmed.len + 2 > DNS_MAX_NAME_LEN) {
        return DnsWireError.NameTooLong;
    }

    let idx: usize = 0;
    while (idx < trimmed.len) {
        const rest = trimmed[idx..];
        if (entries := comp_entries) {
            if (ptr := findSuffixOffset(entries.items, rest)) {
                const hi: u8 = @as(u8, 0xC0) | @as(u8, @truncate((ptr >> 8) & 0x3F));
                const lo: u8 = @truncate(ptr & 0xFF);
                try buf.append(alloc, hi);
                try buf.append(alloc, lo);
                return;
            }
            if (buf.items.len < 0x3FFF) {
                try entries.append(alloc, CompEntry{
                    .suffix = rest,
                    .offset = @intCast(buf.items.len),
                });
            }
        }
        const dot_pos = std.mem.indexOfScalarPos(u8, trimmed, idx, '.') orelse trimmed.len;
        const label = trimmed[idx..dot_pos];
        if (label.len == 0 or label.len > DNS_MAX_LABEL_LEN) {
            return DnsWireError.InvalidLabel;
        }
        try buf.append(alloc, @intCast(label.len));
        try buf.appendSlice(alloc, label);
        idx = if (dot_pos < trimmed.len) dot_pos + 1 else trimmed.len;
    }
    try buf.append(alloc, 0);
}

fn appendU16(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, val: u16): !void {
    let tmp: [2]u8 = undefined;
    std.mem.writeInt(u16, &tmp, val, .big);
    try buf.appendSlice(alloc, &tmp);
}

fn appendU32(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, val: u32): !void {
    let tmp: [4]u8 = undefined;
    std.mem.writeInt(u32, &tmp, val, .big);
    try buf.appendSlice(alloc, &tmp);
}

fn encodeRecord(
    buf: *std.ArrayList(u8),
    alloc: std.mem.Allocator,
    rr: DnsResourceRecord,
    comp_entries: *std.ArrayList(CompEntry)?,
): DnsWireError!void {
    try encodeDnsName(buf, alloc, rr.name, comp_entries);
    try appendU16(buf, alloc, rr.rtype);
    try appendU16(buf, alloc, rr.rclass);
    try appendU32(buf, alloc, rr.ttl);

    const rdlen_pos = buf.items.len;
    try appendU16(buf, alloc, 0);
    const data_start = buf.items.len;

    switch (rr.rdata) {
        .a => |ip| try buf.appendSlice(alloc, &ip),
        .aaaa => |ip6| try buf.appendSlice(alloc, &ip6),
        .cname, .ns, .ptr => |target| try encodeDnsName(buf, alloc, target, comp_entries),
        .mx => |m| {
            try appendU16(buf, alloc, m.preference);
            try encodeDnsName(buf, alloc, m.exchange, comp_entries);
        },
        .txt => |items| {
            for (chunk of items) {
                if (chunk.len > 255) {
                    return DnsWireError.InvalidRdata;
                }
                try buf.append(alloc, @intCast(chunk.len));
                try buf.appendSlice(alloc, chunk);
            }
        },
        .srv => |s| {
            try appendU16(buf, alloc, s.priority);
            try appendU16(buf, alloc, s.weight);
            try appendU16(buf, alloc, s.port);
            try encodeDnsName(buf, alloc, s.target, comp_entries);
        },
        .soa => |s| {
            try encodeDnsName(buf, alloc, s.mname, comp_entries);
            try encodeDnsName(buf, alloc, s.rname, comp_entries);
            try appendU32(buf, alloc, s.serial);
            try appendU32(buf, alloc, s.refresh);
            try appendU32(buf, alloc, s.retry);
            try appendU32(buf, alloc, s.expire);
            try appendU32(buf, alloc, s.minimum);
        },
        .opt, .raw => |b| try buf.appendSlice(alloc, b),
    }

    const rdata_len = buf.items.len - data_start;
    if (rdata_len > 65535) {
        return DnsWireError.InvalidRdata;
    }
    std.mem.writeInt(u16, buf.items[rdlen_pos .. rdlen_pos + 2], @intCast(rdata_len), .big);
}

pub fn encodeDnsPacket(alloc: std.mem.Allocator, pkt: DnsPacket, compress_names: bool): DnsWireError![]u8 {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    let comp_list: std.ArrayList(CompEntry) = .empty;
    defer comp_list.deinit(alloc);
    const comp_ptr: *std.ArrayList(CompEntry)? = if (compress_names) &comp_list else null;

    let hdr = pkt.header;
    hdr.qd_count = @intCast(pkt.questions.len);
    hdr.an_count = @intCast(pkt.answers.len);
    hdr.ns_count = @intCast(pkt.authorities.len);
    hdr.ar_count = @intCast(pkt.additionals.len);
    const hdr_bytes = hdr.encode();
    try out.appendSlice(alloc, &hdr_bytes);

    for (q of pkt.questions) {
        try encodeDnsName(&out, alloc, q.name, comp_ptr);
        try appendU16(&out, alloc, q.qtype);
        try appendU16(&out, alloc, q.qclass);
    }
    for (rr of pkt.answers) {
        try encodeRecord(&out, alloc, rr, comp_ptr);
    }
    for (rr of pkt.authorities) {
        try encodeRecord(&out, alloc, rr, comp_ptr);
    }
    for (rr of pkt.additionals) {
        try encodeRecord(&out, alloc, rr, comp_ptr);
    }

    return out.toOwnedSlice(alloc);
}

fn decodeRdata(alloc: std.mem.Allocator, packet: []const u8, rtype: u16, rdata_start: usize, rdlen: usize): DnsWireError!DnsRdata {
    if (rdata_start + rdlen > packet.len) {
        return DnsWireError.TruncatedRecord;
    }
    const slice = packet[rdata_start .. rdata_start + rdlen];
    const rt: DnsRecordType = @enumFromInt(rtype);
    switch (rt) {
        .A => {
            if (rdlen != 4) { return DnsWireError.InvalidRdata; }
            let ip: [4]u8 = undefined;
            @memcpy(&ip, slice);
            return DnsRdata{ .a = ip };
        },
        .AAAA => {
            if (rdlen != 16) { return DnsWireError.InvalidRdata; }
            let ip6: [16]u8 = undefined;
            @memcpy(&ip6, slice);
            return DnsRdata{ .aaaa = ip6 };
        },
        .CNAME => {
            const dn = try decodeDnsName(alloc, packet, rdata_start);
            if (dn.next_offset != rdata_start + rdlen) {
                alloc.free(dn.name);
                return DnsWireError.InvalidRdata;
            }
            return DnsRdata{ .cname = dn.name };
        },
        .NS => {
            const dn = try decodeDnsName(alloc, packet, rdata_start);
            if (dn.next_offset != rdata_start + rdlen) {
                alloc.free(dn.name);
                return DnsWireError.InvalidRdata;
            }
            return DnsRdata{ .ns = dn.name };
        },
        .PTR => {
            const dn = try decodeDnsName(alloc, packet, rdata_start);
            if (dn.next_offset != rdata_start + rdlen) {
                alloc.free(dn.name);
                return DnsWireError.InvalidRdata;
            }
            return DnsRdata{ .ptr = dn.name };
        },
        .MX => {
            if (rdlen < 3) { return DnsWireError.InvalidRdata; }
            const pref = std.mem.readInt(u16, slice[0..2], .big);
            const dn = try decodeDnsName(alloc, packet, rdata_start + 2);
            if (dn.next_offset != rdata_start + rdlen) {
                alloc.free(dn.name);
                return DnsWireError.InvalidRdata;
            }
            return DnsRdata{ .mx = MxData{ .preference = pref, .exchange = dn.name } };
        },
        .TXT => {
            let parts: std.ArrayList(StrSlice) = .empty;
            errdefer {
                for (p of parts.items) { alloc.free(p); }
                parts.deinit(alloc);
            }
            let off: usize = 0;
            while (off < slice.len) {
                const slen = @as(usize, slice[off]);
                off += 1;
                if (off + slen > slice.len) { return DnsWireError.InvalidRdata; }
                const copy = try alloc.dupe(u8, slice[off .. off + slen]);
                try parts.append(alloc, copy);
                off += slen;
            }
            return DnsRdata{ .txt = try parts.toOwnedSlice(alloc) };
        },
        .SRV => {
            if (rdlen < 7) { return DnsWireError.InvalidRdata; }
            const prio = std.mem.readInt(u16, slice[0..2], .big);
            const wt = std.mem.readInt(u16, slice[2..4], .big);
            const port = std.mem.readInt(u16, slice[4..6], .big);
            const dn = try decodeDnsName(alloc, packet, rdata_start + 6);
            if (dn.next_offset != rdata_start + rdlen) {
                alloc.free(dn.name);
                return DnsWireError.InvalidRdata;
            }
            return DnsRdata{ .srv = SrvData{ .priority = prio, .weight = wt, .port = port, .target = dn.name } };
        },
        .SOA => {
            const m = try decodeDnsName(alloc, packet, rdata_start);
            errdefer alloc.free(m.name);
            const r = try decodeDnsName(alloc, packet, m.next_offset);
            errdefer alloc.free(r.name);
            if (r.next_offset + 20 != rdata_start + rdlen) {
                return DnsWireError.InvalidRdata;
            }
            const tail = packet[r.next_offset .. r.next_offset + 20];
            return DnsRdata{ .soa = SoaData{
                .mname = m.name,
                .rname = r.name,
                .serial = std.mem.readInt(u32, tail[0..4], .big),
                .refresh = std.mem.readInt(u32, tail[4..8], .big),
                .retry = std.mem.readInt(u32, tail[8..12], .big),
                .expire = std.mem.readInt(u32, tail[12..16], .big),
                .minimum = std.mem.readInt(u32, tail[16..20], .big),
            } };
        },
        .OPT => return DnsRdata{ .opt = try alloc.dupe(u8, slice) },
        _ => return DnsRdata{ .raw = try alloc.dupe(u8, slice) },
    }
}

fn decodeRecordList(
    alloc: std.mem.Allocator,
    packet: []const u8,
    pos_ptr: *usize,
    count: u16,
): DnsWireError![]DnsResourceRecord {
    let list: std.ArrayList(DnsResourceRecord) = .empty;
    errdefer {
        for (rr of list.items) { rr.deinit(alloc); }
        list.deinit(alloc);
    }
    let i: u16 = 0;
    while (i < count) : (i += 1) {
        const dn = try decodeDnsName(alloc, packet, pos_ptr.*);
        errdefer alloc.free(dn.name);
        if (dn.next_offset + 10 > packet.len) {
            return DnsWireError.TruncatedRecord;
        }
        const meta = packet[dn.next_offset .. dn.next_offset + 10];
        const rtype = std.mem.readInt(u16, meta[0..2], .big);
        const rclass = std.mem.readInt(u16, meta[2..4], .big);
        const ttl = std.mem.readInt(u32, meta[4..8], .big);
        const rdlen = @as(usize, std.mem.readInt(u16, meta[8..10], .big));
        const rdata_start = dn.next_offset + 10;
        const rdata = try decodeRdata(alloc, packet, rtype, rdata_start, rdlen);
        pos_ptr.* = rdata_start + rdlen;
        try list.append(alloc, DnsResourceRecord{
            .name = dn.name,
            .rtype = rtype,
            .rclass = rclass,
            .ttl = ttl,
            .rdata = rdata,
        });
    }
    return list.toOwnedSlice(alloc);
}

pub fn decodeDnsPacket(alloc: std.mem.Allocator, packet: []const u8): DnsWireError!DnsPacket {
    const hdr = try DnsHeader.decode(packet);
    let pos: usize = DNS_HEADER_LEN;

    let qlist: std.ArrayList(DnsQuestion) = .empty;
    errdefer {
        for (q of qlist.items) { q.deinit(alloc); }
        qlist.deinit(alloc);
    }
    let i: u16 = 0;
    while (i < hdr.qd_count) : (i += 1) {
        const dn = try decodeDnsName(alloc, packet, pos);
        errdefer alloc.free(dn.name);
        if (dn.next_offset + 4 > packet.len) {
            return DnsWireError.TruncatedRecord;
        }
        const qtype = std.mem.readInt(u16, packet[dn.next_offset .. dn.next_offset + 2], .big);
        const qclass = std.mem.readInt(u16, packet[dn.next_offset + 2 .. dn.next_offset + 4], .big);
        pos = dn.next_offset + 4;
        try qlist.append(alloc, DnsQuestion{
            .name = dn.name,
            .qtype = qtype,
            .qclass = qclass,
        });
    }
    const questions = try qlist.toOwnedSlice(alloc);
    errdefer {
        for (q of questions) { q.deinit(alloc); }
        alloc.free(questions);
    }

    const answers = try decodeRecordList(alloc, packet, &pos, hdr.an_count);
    errdefer {
        for (r of answers) { r.deinit(alloc); }
        alloc.free(answers);
    }

    const authorities = try decodeRecordList(alloc, packet, &pos, hdr.ns_count);
    errdefer {
        for (r of authorities) { r.deinit(alloc); }
        alloc.free(authorities);
    }

    const additionals = try decodeRecordList(alloc, packet, &pos, hdr.ar_count);
    return DnsPacket{
        .header = hdr,
        .questions = questions,
        .answers = answers,
        .authorities = authorities,
        .additionals = additionals,
    };
}
