// Type definitions for CP

export class Ipv4Result {
    ip: u32,
    valid: bool,
}

export class Ipv4Cidr {
    network: u32,
    broadcast: u32,
    mask: u32,
    prefix_len: u8,
    valid: bool,
}

export fn parseOctetStrict(s: string): i32;
export fn parseIpv4(s: string): Ipv4Result;
export fn appendU8Dec(out: []u8, pos: usize, v: u8): i64;
export fn formatIpv4(ip: u32, out: []u8): usize;
export fn parseCidr(s: string): Ipv4Cidr;
export fn cidrContains(cidr: Ipv4Cidr, ip: u32): bool;
export fn cidrOverlaps(a: Ipv4Cidr, b: Ipv4Cidr): bool;
export fn isPrivateIpv4(ip: u32): bool;
export fn isLoopbackIpv4(ip: u32): bool;
export fn isLinkLocalIpv4(ip: u32): bool;
export fn isPublicIpv4(ip: u32): bool;
