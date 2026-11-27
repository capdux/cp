// Type definitions for CP

import { userinfoSet, pathSet, querySet, fragmentSet, percentEncode, percentDecode } from "../../percent-encoding/src/index.cp";
import { domainToAscii } from "../../idna/src/index.cp";
import { toLowerByte, isAlpha, isDigit, isHexDigit } from "cap:ascii";
import { bytesEq } from "cap:bin";
export fn wuIsSchemeChar(ch: u8): bool;
export fn wuAppendBytes(out: []u8, pos: &mut usize, src: &[]u8): bool;
export fn wuAppendByte(out: []u8, pos: &mut usize, b: u8): bool;
export fn wuAppendPort(out: []u8, pos: &mut usize, port: i32): bool;
export fn wuIsSpecialScheme(scheme: &[]u8): bool;
export fn wuIsOriginScheme(scheme: &[]u8): bool;
export fn wuIsForbiddenHostByte(b: u8): bool;
export fn wuIsSingleDotSegment(seg: &[]u8): bool;
export fn wuIsDoubleDotSegment(seg: &[]u8): bool;
export fn wuTrimBounds(input: string, out_start: &mut usize, out_end: &mut usize);
export fn wuFindSchemeColon(s: &[]u8): i64;
export fn wuNormalizePath(raw: &[]u8, is_special: bool, out: []u8): usize;
export fn wuParsePortStr(port_str: &[]u8): i32;
export fn wuParseIpv6Host(inner: &[]u8, out: []u8): i64;
export fn defaultPortForScheme(scheme: string): i32;
export class WhatwgUrl {
    scheme: [32]u8,
    scheme_len: usize,
    username: [128]u8,
    username_len: usize,
    password: [128]u8,
    password_len: usize,
    host: [256]u8,
    host_len: usize,
    port: i32,
    path: [512]u8,
    path_len: usize,
    query: [512]u8,
    query_len: usize,
    fragment: [256]u8,
    fragment_len: usize,
    has_authority: bool,
    has_query: bool,
    has_fragment: bool,
    ok: bool,
    fn invalid(): WhatwgUrl;
    fn schemeStr(): &[]u8;
    fn usernameStr(): &[]u8;
    fn passwordStr(): &[]u8;
    fn hostStr(): &[]u8;
    fn pathStr(): &[]u8;
    fn queryStr(): &[]u8;
    fn fragmentStr(): &[]u8;
    fn effectivePort(): i32;
    fn parse(input: string): WhatwgUrl;
    fn join(base: WhatwgUrl, rel: string): WhatwgUrl;
    fn toStr(out: []u8): usize;
    fn origin(out: []u8): usize;
}

export fn wuCloneUrl(src: WhatwgUrl): WhatwgUrl;
