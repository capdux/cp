// Type definitions for CP

export fn ipaHexVal(ch: u8): i16;
export fn ipaMask32(prefix: u8): u32;
export fn ipaMask16(bits: u8): u16;
export fn ipaWriteU8Dec(dst: []u8, pos: usize, valIn: u8): usize;
export fn ipaWriteU16Hex(dst: []u8, pos: usize, valIn: u16): usize;
export fn ipaParsePrefix(s: string, maxPrefix: u8): i16;
export fn ipaInvalidV4(): Ipv4Addr;
export fn ipaInvalidV6(): Ipv6Addr;
export class Ipv4Addr {
    octets: [4]u8,
    ok: bool,
    fn fromU32(val: u32): Ipv4Addr;
    fn subnetMaskFromPrefixLength(prefix: u8): Ipv4Addr;
    fn kind(): string;
    fn toU32(): u32;
    fn toByteArray(): [4]u8;
    fn equals(other: Ipv4Addr): bool;
    fn toStr(out: []u8): usize;
    fn matchCidr(net: Ipv4Addr, prefix: u8): bool;
    fn prefixLengthFromSubnetMask(): i8;
    fn range(): string;
    fn toIpv4MappedIpv6(): Ipv6Addr;
}

export class Ipv6Addr {
    parts: [8]u16,
    ok: bool,
    fn kind(): string;
    fn toByteArray(): [16]u8;
    fn equals(other: Ipv6Addr): bool;
    fn toStr(out: []u8): usize;
    fn toNormalizedStr(out: []u8): usize;
    fn matchCidr(net: Ipv6Addr, prefix: u8): bool;
    fn isIpv4Mapped(): bool;
    fn toIpv4(): Ipv4Addr;
    fn range(): string;
}

export class CidrV4 {
    addr: Ipv4Addr,
    prefix: u8,
    ok: bool,
    fn matches(ip: Ipv4Addr): bool;
}

export class CidrV6 {
    addr: Ipv6Addr,
    prefix: u8,
    ok: bool,
    fn matches(ip: Ipv6Addr): bool;
}

export fn parseIpv4(s: string): Ipv4Addr;
export fn parseIpv6(s: string): Ipv6Addr;
export fn parseCidrV4(s: string): CidrV4;
export fn parseCidrV6(s: string): CidrV6;
export fn isValidIp(s: string): bool;
export fn isValidIpv4(s: string): bool;
export fn isValidIpv6(s: string): bool;
export fn ipKind(s: string): string;
