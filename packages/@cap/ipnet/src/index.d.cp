// Type definitions for CP

export fn ipnParseIpv4Range(s: string, start: usize, end: usize): u32?;
export fn ipnParsePrefixRange(s: string, start: usize, end: usize): u8?;
export fn ipnWriteU8Dec(v: u8, out: []u8, pos: usize): usize;
export fn ipnNetmask(prefix_len: u8): u32;
export fn parseIpv4(s: string): u32?;
export fn formatIpv4(ip: u32, out: []u8): usize;
export class Ipv4Net {
    addr: u32,
    prefix_len: u8,
    fn new(addr: u32, prefix_len: u8): Ipv4Net?;
    fn parse(cidr: string): Ipv4Net?;
    fn netmask(): u32;
    fn hostmask(): u32;
    fn network(): u32;
    fn broadcast(): u32;
    fn trunc(): Ipv4Net;
    fn containsIp(ip: u32): bool;
    fn containsNet(other: Ipv4Net): bool;
    fn supernet(): Ipv4Net?;
    fn hostCount(): u64;
    fn formatCidr(out: []u8): usize;
}

