// Type definitions for CP

import { isDigit } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";
export class ClfRecord {
    host_start: usize,
    host_len: usize,
    user_start: usize,
    user_len: usize,
    time_start: usize,
    time_len: usize,
    method_start: usize,
    method_len: usize,
    path_start: usize,
    path_len: usize,
    proto_start: usize,
    proto_len: usize,
    status: u16,
    bytes: i64,
    referer_start: usize,
    referer_len: usize,
    ua_start: usize,
    ua_len: usize,
    is_combined: bool,
    valid: bool,
}

export fn invalidClf(): ClfRecord;
export fn isClfSpace(b: u8): bool;
export fn isLineEnd(b: u8): bool;
export fn parseClf(line: &[]u8): ClfRecord;
export fn pushSlice(dst: []u8, pos: usize, src: &[]u8): i64;
export fn pushU64(dst: []u8, pos: usize, val: u64): i64;
export fn formatClf(host: &[]u8, user: &[]u8, time: &[]u8, method: &[]u8, path: &[]u8, proto: &[]u8, status: u16, bytes: i64, dst: []u8): i64;
