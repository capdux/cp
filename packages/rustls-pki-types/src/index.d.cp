// Type definitions for CP

import { decodeLabel, decodePem } from "../../pem-rfc7468/src/index.cp";
import { isDigit, isAlpha, hexVal, eqIgnoreCase, toLower } from "cap:ascii";
import { bytesEq } from "cap:bin";
export fn pkiIsDnsChar(ch: u8): bool;
export fn pkiEqIgnoreCase(a: string, b: string): bool;
export fn pkiClassifyPemLabel(label: &[]u8): u8;
export fn pkiParseIpv4Slice(s: string, start: usize, end: usize, out: &mut [4]u8): bool;
export fn pkiParseIpv6Slice(input: string, out: &mut [16]u8): bool;
export class DnsName {
    buf: [253]u8,
    len: usize,
    valid: bool,
    fn invalid(): DnsName;
    fn parse(input: string): DnsName;
    fn isValid(): bool;
    fn eqIgnoreCase(other: DnsName): bool;
    fn eqIgnoreCaseStr(other: string): bool;
    fn labelCount(): usize;
    fn isSubdomainOf(parent: DnsName): bool;
    fn toLowercase(out: []u8): usize;
}

export fn isValidDnsName(input: string): bool;
export fn parseDnsName(input: string): DnsName;
export class ServerName {
    kind: u8,
    dns: DnsName,
    ipv4: [4]u8,
    ipv6: [16]u8,
    fn invalid(): ServerName;
    fn fromDns(dns: DnsName): ServerName;
    fn fromIpv4(octets: [4]u8): ServerName;
    fn fromIpv6(octets: [16]u8): ServerName;
    fn parse(input: string): ServerName;
    fn isValid(): bool;
    fn isDns(): bool;
    fn isIpv4(): bool;
    fn isIpv6(): bool;
    fn isIp(): bool;
}

export fn parseServerName(input: string): ServerName;
export class UnixTime {
    seconds: u64,
    fn epoch(): UnixTime;
    fn fromSeconds(secs: u64): UnixTime;
    fn asSeconds(): u64;
    fn addSeconds(delta: u64): UnixTime;
    fn subSeconds(delta: u64): UnixTime;
    fn isBefore(other: UnixTime): bool;
    fn isAfter(other: UnixTime): bool;
    fn equals(other: UnixTime): bool;
    fn isWithin(not_before: UnixTime, not_after: UnixTime): bool;
}

export class PkiPemResult {
    kind: u8,
    der_len: usize,
    err: i64,
    fn isOk(): bool;
    fn isCertificate(): bool;
    fn isPrivateKey(): bool;
}

export fn detectPemKind(pem: string): u8;
export fn decodePemDer(pem: string, der_out: []u8): PkiPemResult;
export fn pemKindLabel(kind: u8): string;
export fn isPrivateKeyKind(kind: u8): bool;
