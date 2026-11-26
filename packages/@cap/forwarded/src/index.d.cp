// Type definitions for CP

export class ForwardedHop {
    for_addr: string,
    by_addr: string,
    host: string,
    proto: string,
    fn empty(): ForwardedHop;
}

export class ForwardedChain {
    hops: [MAX_HOPS]ForwardedHop,
    count: usize,
    fn empty(): ForwardedChain;
    fn hopAt(i: usize): ForwardedHop;
    fn clientIp(): string;
    fn clientProto(): string;
    fn clientHost(): string;
    fn ipAtTrustDepth(trusted_proxies: usize): string;
}

export fn isSpace(c: u8): bool;
export fn isDigit(c: u8): bool;
export fn toLowerAscii(c: u8): u8;
export fn trim(s: string): string;
export fn unquote(raw: string): string;
export fn eqIgnoreCase(a: string, lower_lit: string): bool;
export fn stripAddr(raw: string): string;
export fn parseForwardedElement(elem: string): ForwardedHop;
export fn parseForwarded(header: string): ForwardedChain;
export fn parseXForwardedFor(xff: string, remote_addr: string): ForwardedChain;
export fn extractClientIp(xff: string, remote_addr: string, trusted_hops: usize): string;
export fn hexVal(c: u8): i8;
export fn isPrivateIpv4(s: string): bool;
export fn isPrivateIp(ip: string): bool;
