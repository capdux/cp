// Type definitions for CP

import { eqAscii } from "../../unicase/src/index.cp";
export class ForwardedEntry {
    for_val: string,
    by_val: string,
    host: string,
    proto: string,
}

export fn fwdIsSpace(c: u8): bool;
export fn fwdIsDigit(c: u8): bool;
export fn fwdTrim(s: string): string;
export fn fwdUnquote(s: string): string;
export fn fwdStrEq(a: string, b: string): bool;
export fn fwdIsValidIpv4Loopback(s: string): bool;
export fn fwdParseElement(seg: string, entry_out: &mut ForwardedEntry): bool;
export fn parseXForwardedFor(remote_addr: string, xff_header: string, out_addrs: []string): usize;
export fn extractClientIp(remote_addr: string, xff_header: string, trust_hops: usize): string;
export fn parseForwardedHeader(header: string, out_entries: []ForwardedEntry): usize;
export fn isLoopbackIp(ip: string): bool;
