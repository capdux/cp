// Type definitions for CP

import { bytesEq, readU16BE, writeU16BE } from "cap:bin";
import { writeStr, writeU64 } from "cap:fmt";
export class ProxyV1Header {
    proto: u8,
    src_ip: string,
    dst_ip: string,
    src_port: u16,
    dst_port: u16,
    consumed: usize,
}

export class ProxyV2Header {
    is_proxy: bool,
    family: u8,
    transport: u8,
    payload_len: u16,
    src_ipv4: [4]u8,
    dst_ipv4: [4]u8,
    src_port: u16,
    dst_port: u16,
    consumed: usize,
}

export fn isV1Prefix(data: &[]u8): bool;
export fn isV2Signature(data: &[]u8): bool;
export fn parsePortStrict(s: string): i32;
export fn parseV1(data: string, out: &mut ProxyV1Header): i32;
export fn appendSlice(out: []u8, pos: usize, s: string): i64;
export fn appendU16(out: []u8, pos: usize, v: u16): i64;
export fn encodeV1(proto: u8, src_ip: string, dst_ip: string, src_port: u16, dst_port: u16, out: []u8): usize;
export fn parseV2(data: &[]u8, out: &mut ProxyV2Header): i32;
export fn encodeV2Ipv4(src_ip: [4]u8, dst_ip: [4]u8, src_port: u16, dst_port: u16, out: []u8): usize;
