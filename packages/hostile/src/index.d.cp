// Type definitions for CP

import { eqIgnoreCase, findByte } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class HostEntry {
    ip: string,
    hostname: string,
    is_ipv6: bool,
    fn empty(): HostEntry;
}

export class HostsTable {
    entries: [MAX_HOST_ENTRIES]HostEntry,
    count: usize,
    fn empty(): HostsTable;
    fn entryAt(i: usize): HostEntry;
}

export fn isBlank(c: u8): bool;
export fn parseHosts(content: string): HostsTable;
export fn lookupIp(table: HostsTable, hostname: string): string?;
export fn lookupIpByFamily(table: HostsTable, hostname: string, ipv6: bool): string?;
export fn reverseLookup(table: HostsTable, ip: string): string?;
export fn pushByte(out: []u8, pos: usize, b: u8): usize;
export fn formatHosts(table: HostsTable, out: []u8): string;
