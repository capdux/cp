// Source: Absorbed from /data/dev/bun/src/dns.zig | Synced: 2026-11-21T16:36:24Z (IANA DNS RRType, Class, RCODE & SRV/CAA Helpers)

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) {
        start += 1;
    }
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn parseU16Dec(s: []const u8): u16? {
    if (s.len == 0 || s.len > 5) {
        return null;
    }
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        acc = acc * 10 + u32(c - '0');
        if (acc > 65535) {
            return null;
        }
        i += 1;
    }
    return acc as u16;
}

fn isValidDomainTarget(s: []const u8): bool {
    if (s.len == 0 || s.len > 255) {
        return false;
    }
    if (s.len == 1 && s[0] == '.') {
        return true;
    }
    let label_len: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '.') {
            if (label_len == 0 || label_len > 63) {
                return false;
            }
            label_len = 0;
        } else if (
            (c >= 'a' && c <= 'z')
                || (c >= 'A' && c <= 'Z')
                || (c >= '0' && c <= '9')
                || c == '-'
                || c == '_'
        ) {
            label_len += 1;
            if (label_len > 63) {
                return false;
            }
        } else {
            return false;
        }
        i += 1;
    }
    return true;
}

fn nextToken(s: []const u8, pos: *usize): ([]const u8)? {
    while (pos.* < s.len && isSpace(s[pos.*])) {
        pos.* += 1;
    }
    if (pos.* >= s.len) {
        return null;
    }
    const start = pos.*;
    while (pos.* < s.len && !isSpace(s[pos.*])) {
        pos.* += 1;
    }
    return s[start..pos.*];
}

/// IANA DNS 资源记录类型（RRType）
export enum DnsRrType {
    a,
    ns,
    cname,
    soa,
    ptr,
    mx,
    txt,
    aaaa,
    srv,
    opt,
    ds,
    rrsig,
    nsec,
    dnskey,
    svcb,
    https,
    any_q,
    caa,

    static fn fromU16(code: u16): DnsRrType? {
        return switch (code) {
            1 => DnsRrType.a,
            2 => DnsRrType.ns,
            5 => DnsRrType.cname,
            6 => DnsRrType.soa,
            12 => DnsRrType.ptr,
            15 => DnsRrType.mx,
            16 => DnsRrType.txt,
            28 => DnsRrType.aaaa,
            33 => DnsRrType.srv,
            41 => DnsRrType.opt,
            43 => DnsRrType.ds,
            46 => DnsRrType.rrsig,
            47 => DnsRrType.nsec,
            48 => DnsRrType.dnskey,
            64 => DnsRrType.svcb,
            65 => DnsRrType.https,
            255 => DnsRrType.any_q,
            257 => DnsRrType.caa,
            else => null,
        };
    }

    fn toU16(): u16 {
        return switch (_) {
            .a => 1,
            .ns => 2,
            .cname => 5,
            .soa => 6,
            .ptr => 12,
            .mx => 15,
            .txt => 16,
            .aaaa => 28,
            .srv => 33,
            .opt => 41,
            .ds => 43,
            .rrsig => 46,
            .nsec => 47,
            .dnskey => 48,
            .svcb => 64,
            .https => 65,
            .any_q => 255,
            .caa => 257,
        };
    }

    static fn parseName(raw: []const u8): DnsRrType? {
        const n = trimAscii(raw);
        if (eqlIgnoreCase(n, "A")) return DnsRrType.a;
        if (eqlIgnoreCase(n, "NS")) return DnsRrType.ns;
        if (eqlIgnoreCase(n, "CNAME")) return DnsRrType.cname;
        if (eqlIgnoreCase(n, "SOA")) return DnsRrType.soa;
        if (eqlIgnoreCase(n, "PTR")) return DnsRrType.ptr;
        if (eqlIgnoreCase(n, "MX")) return DnsRrType.mx;
        if (eqlIgnoreCase(n, "TXT")) return DnsRrType.txt;
        if (eqlIgnoreCase(n, "AAAA")) return DnsRrType.aaaa;
        if (eqlIgnoreCase(n, "SRV")) return DnsRrType.srv;
        if (eqlIgnoreCase(n, "OPT")) return DnsRrType.opt;
        if (eqlIgnoreCase(n, "DS")) return DnsRrType.ds;
        if (eqlIgnoreCase(n, "RRSIG")) return DnsRrType.rrsig;
        if (eqlIgnoreCase(n, "NSEC")) return DnsRrType.nsec;
        if (eqlIgnoreCase(n, "DNSKEY")) return DnsRrType.dnskey;
        if (eqlIgnoreCase(n, "SVCB")) return DnsRrType.svcb;
        if (eqlIgnoreCase(n, "HTTPS")) return DnsRrType.https;
        if (eqlIgnoreCase(n, "ANY") || eqlIgnoreCase(n, "*")) return DnsRrType.any_q;
        if (eqlIgnoreCase(n, "CAA")) return DnsRrType.caa;
        return null;
    }

    fn toName(): []const u8 {
        return switch (_) {
            .a => "A",
            .ns => "NS",
            .cname => "CNAME",
            .soa => "SOA",
            .ptr => "PTR",
            .mx => "MX",
            .txt => "TXT",
            .aaaa => "AAAA",
            .srv => "SRV",
            .opt => "OPT",
            .ds => "DS",
            .rrsig => "RRSIG",
            .nsec => "NSEC",
            .dnskey => "DNSKEY",
            .svcb => "SVCB",
            .https => "HTTPS",
            .any_q => "ANY",
            .caa => "CAA",
        };
    }
}

/// IANA DNS 响应码（RCODE）
export enum DnsRcode {
    noerror,
    formerr,
    servfail,
    nxdomain,
    notimp,
    refused,
    yxdomain,
    nxrrset,
    notauth,
    notzone,

    static fn fromU16(code: u16): DnsRcode? {
        return switch (code) {
            0 => DnsRcode.noerror,
            1 => DnsRcode.formerr,
            2 => DnsRcode.servfail,
            3 => DnsRcode.nxdomain,
            4 => DnsRcode.notimp,
            5 => DnsRcode.refused,
            6 => DnsRcode.yxdomain,
            8 => DnsRcode.nxrrset,
            9 => DnsRcode.notauth,
            10 => DnsRcode.notzone,
            else => null,
        };
    }

    fn toU16(): u16 {
        return switch (_) {
            .noerror => 0,
            .formerr => 1,
            .servfail => 2,
            .nxdomain => 3,
            .notimp => 4,
            .refused => 5,
            .yxdomain => 6,
            .nxrrset => 8,
            .notauth => 9,
            .notzone => 10,
        };
    }

    fn toName(): []const u8 {
        return switch (_) {
            .noerror => "NOERROR",
            .formerr => "FORMERR",
            .servfail => "SERVFAIL",
            .nxdomain => "NXDOMAIN",
            .notimp => "NOTIMP",
            .refused => "REFUSED",
            .yxdomain => "YXDOMAIN",
            .nxrrset => "NXRRSET",
            .notauth => "NOTAUTH",
            .notzone => "NOTZONE",
        };
    }

    fn isError(): bool {
        return _ != DnsRcode.noerror;
    }
}

/// IANA DNS 查询类（QCLASS / CLASS）
export enum DnsClass {
    in_class,
    ch,
    hs,
    any_class,

    static fn fromU16(code: u16): DnsClass? {
        return switch (code) {
            1 => DnsClass.in_class,
            3 => DnsClass.ch,
            4 => DnsClass.hs,
            255 => DnsClass.any_class,
            else => null,
        };
    }

    fn toU16(): u16 {
        return switch (_) {
            .in_class => 1,
            .ch => 3,
            .hs => 4,
            .any_class => 255,
        };
    }

    static fn parseName(raw: []const u8): DnsClass? {
        const n = trimAscii(raw);
        if (eqlIgnoreCase(n, "IN")) return DnsClass.in_class;
        if (eqlIgnoreCase(n, "CH") || eqlIgnoreCase(n, "CHAOS")) return DnsClass.ch;
        if (eqlIgnoreCase(n, "HS") || eqlIgnoreCase(n, "HESIOD")) return DnsClass.hs;
        if (eqlIgnoreCase(n, "ANY") || eqlIgnoreCase(n, "*")) return DnsClass.any_class;
        return null;
    }

    fn toName(): []const u8 {
        return switch (_) {
            .in_class => "IN",
            .ch => "CH",
            .hs => "HS",
            .any_class => "ANY",
        };
    }
}

/// RFC 2782 DNS SRV 文本记录（`<priority> <weight> <port> <target>`）
export struct SrvRecord {
    priority: u16;
    weight: u16;
    port: u16;
    target: []const u8;

    static fn parseText(rdata: []const u8): SrvRecord? {
        let pos: usize = 0;
        const prio_s = nextToken(rdata, &pos) ?? return null;
        const weight_s = nextToken(rdata, &pos) ?? return null;
        const port_s = nextToken(rdata, &pos) ?? return null;
        const target = nextToken(rdata, &pos) ?? return null;

        // 禁止多余尾随 token
        if (nextToken(rdata, &pos) != null) {
            return null;
        }

        const priority = parseU16Dec(prio_s) ?? return null;
        const weight = parseU16Dec(weight_s) ?? return null;
        const port = parseU16Dec(port_s) ?? return null;

        if (!isValidDomainTarget(target)) {
            return null;
        }

        return SrvRecord{
            priority: priority,
            weight: weight,
            port: port,
            target: target,
        };
    }
}

/// RFC 1035 DNS MX 文本记录（`<preference> <exchange>`）
export struct MxRecord {
    preference: u16;
    exchange: []const u8;

    static fn parseText(rdata: []const u8): MxRecord? {
        let pos: usize = 0;
        const pref_s = nextToken(rdata, &pos) ?? return null;
        const exchange = nextToken(rdata, &pos) ?? return null;

        if (nextToken(rdata, &pos) != null) {
            return null;
        }

        const preference = parseU16Dec(pref_s) ?? return null;
        if (!isValidDomainTarget(exchange)) {
            return null;
        }

        return MxRecord{
            preference: preference,
            exchange: exchange,
        };
    }
}
