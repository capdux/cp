// Type definitions for CP

export class IpSpan {
    start: usize,
    len: usize,
}

export class PaIpv4 {
    ok: bool,
    o0: u8,
    o1: u8,
    o2: u8,
    o3: u8,
}

export class PaIpv6Info {
    ok: bool,
    is_loopback: bool,
    first_word: u16,
}

export fn paIsSpace(c: u8): bool;
export fn paToLower(c: u8): u8;
export fn paHexVal(c: u8): i32;
export fn paTrimBounds(s: string): IpSpan;
export fn paStripV4Mapped(s: string): string;
export fn paParseIpv4(s: string): PaIpv4;
export fn paParseIpv6Info(s: string): PaIpv6Info;
export fn isLoopback(ip: string): bool;
export fn isLinkLocal(ip: string): bool;
export fn isUniqueLocal(ip: string): bool;
export fn isTrustedBuiltin(ip: string, preset: u8): bool;
export fn parseXffAddresses(remote_addr: string, xff_header: string, out: []IpSpan): usize;
export fn paCopyToOut(src: string, out: []u8): usize;
export fn resolveClientIp(remote_addr: string, xff_header: string, trust_preset: u8, max_hops: usize, out: []u8): usize;
