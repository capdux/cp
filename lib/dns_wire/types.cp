// Source: cp/lib/dns_wire/types.cp (Pure CP RFC 1035/6891 DNS Wire Types)
const std = @import("std");

pub const DNS_HEADER_LEN: usize = 12;
pub const DNS_MAX_LABEL_LEN: usize = 63;
pub const DNS_MAX_NAME_LEN: usize = 255;
pub const DNS_MAX_POINTER_JUMPS: usize = 32;

pub const DnsWireError = error{
    UnexpectedEof,
    InvalidHeader,
    InvalidLabel,
    NameTooLong,
    CompressionLoop,
    TruncatedRecord,
    InvalidRdata,
    OutOfMemory,
};

pub const DnsOpcode = enum(u4) {
    Query = 0,
    IQuery = 1,
    Status = 2,
    Notify = 4,
    Update = 5,
    _,
};

pub const DnsRcode = enum(u4) {
    NoError = 0,
    FormErr = 1,
    ServFail = 2,
    NXDomain = 3,
    NotImp = 4,
    Refused = 5,
    _,
};

pub const DnsRecordType = enum(u16) {
    A = 1,
    NS = 2,
    CNAME = 5,
    SOA = 6,
    PTR = 12,
    MX = 15,
    TXT = 16,
    AAAA = 28,
    SRV = 33,
    OPT = 41,
    ANY = 255,
    _,
};

pub const DnsClass = enum(u16) {
    IN = 1,
    CH = 3,
    HS = 4,
    ANY = 255,
    _,
};

pub const DnsHeader = struct {
    id: u16,
    qr: bool = false,
    opcode: u4 = 0,
    aa: bool = false,
    tc: bool = false,
    rd: bool = true,
    ra: bool = false,
    ad: bool = false,
    cd: bool = false,
    rcode: u4 = 0,
    qd_count: u16 = 0,
    an_count: u16 = 0,
    ns_count: u16 = 0,
    ar_count: u16 = 0,

    pub fn encode(self: DnsHeader): [12]u8 {
        let out: [12]u8 = undefined;
        std.mem.writeInt(u16, out[0..2], self.id, .big);
        let flags: u16 = 0;
        if (self.qr) { flags |= 0x8000; }
        flags |= (@as(u16, self.opcode) << 11);
        if (self.aa) { flags |= 0x0400; }
        if (self.tc) { flags |= 0x0200; }
        if (self.rd) { flags |= 0x0100; }
        if (self.ra) { flags |= 0x0080; }
        if (self.ad) { flags |= 0x0020; }
        if (self.cd) { flags |= 0x0010; }
        flags |= @as(u16, self.rcode);
        std.mem.writeInt(u16, out[2..4], flags, .big);
        std.mem.writeInt(u16, out[4..6], self.qd_count, .big);
        std.mem.writeInt(u16, out[6..8], self.an_count, .big);
        std.mem.writeInt(u16, out[8..10], self.ns_count, .big);
        std.mem.writeInt(u16, out[10..12], self.ar_count, .big);
        return out;
    }

    pub fn decode(buf: []const u8): DnsWireError!DnsHeader {
        if (buf.len < DNS_HEADER_LEN) {
            return DnsWireError.UnexpectedEof;
        }
        const flags = std.mem.readInt(u16, buf[2..4], .big);
        return DnsHeader{
            .id = std.mem.readInt(u16, buf[0..2], .big),
            .qr = (flags & 0x8000) != 0,
            .opcode = @truncate((flags >> 11) & 0x0F),
            .aa = (flags & 0x0400) != 0,
            .tc = (flags & 0x0200) != 0,
            .rd = (flags & 0x0100) != 0,
            .ra = (flags & 0x0080) != 0,
            .ad = (flags & 0x0020) != 0,
            .cd = (flags & 0x0010) != 0,
            .rcode = @truncate(flags & 0x000F),
            .qd_count = std.mem.readInt(u16, buf[4..6], .big),
            .an_count = std.mem.readInt(u16, buf[6..8], .big),
            .ns_count = std.mem.readInt(u16, buf[8..10], .big),
            .ar_count = std.mem.readInt(u16, buf[10..12], .big),
        };
    }
};

pub const DnsQuestion = struct {
    name: []const u8,
    qtype: u16,
    qclass: u16 = 1,

    pub fn deinit(self: *const DnsQuestion, alloc: std.mem.Allocator) {
        alloc.free(self.name);
    }
};

pub const MxData = struct {
    preference: u16,
    exchange: []const u8,
};

pub const SrvData = struct {
    priority: u16,
    weight: u16,
    port: u16,
    target: []const u8,
};

pub const SoaData = struct {
    mname: []const u8,
    rname: []const u8,
    serial: u32,
    refresh: u32,
    retry: u32,
    expire: u32,
    minimum: u32,
};

pub const DnsRdata = union(enum) {
    a: [4]u8,
    aaaa: [16]u8,
    cname: []const u8,
    ns: []const u8,
    ptr: []const u8,
    mx: MxData,
    txt: [][]const u8,
    srv: SrvData,
    soa: SoaData,
    opt: []const u8,
    raw: []const u8,

    pub fn deinit(self: *const DnsRdata, alloc: std.mem.Allocator) {
        switch (self.*) {
            .a, .aaaa => {},
            .cname, .ns, .ptr => |s| alloc.free(s),
            .mx => |m| alloc.free(m.exchange),
            .txt => |items| {
                for (items) |item| {
                    alloc.free(item);
                }
                alloc.free(items);
            },
            .srv => |s| alloc.free(s.target),
            .soa => |s| {
                alloc.free(s.mname);
                alloc.free(s.rname);
            },
            .opt, .raw => |b| alloc.free(b),
        }
    }
};

pub const DnsResourceRecord = struct {
    name: []const u8,
    rtype: u16,
    rclass: u16 = 1,
    ttl: u32 = 0,
    rdata: DnsRdata,

    pub fn deinit(self: *const DnsResourceRecord, alloc: std.mem.Allocator) {
        alloc.free(self.name);
        self.rdata.deinit(alloc);
    }
};

pub const DnsPacket = struct {
    header: DnsHeader,
    questions: []DnsQuestion,
    answers: []DnsResourceRecord,
    authorities: []DnsResourceRecord,
    additionals: []DnsResourceRecord,

    pub fn deinit(self: *DnsPacket, alloc: std.mem.Allocator) {
        for (q of self.questions) {
            q.deinit(alloc);
        }
        alloc.free(self.questions);
        for (r of self.answers) {
            r.deinit(alloc);
        }
        alloc.free(self.answers);
        for (r of self.authorities) {
            r.deinit(alloc);
        }
        alloc.free(self.authorities);
        for (r of self.additionals) {
            r.deinit(alloc);
        }
        alloc.free(self.additionals);
    }
};
