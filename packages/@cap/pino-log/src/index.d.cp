// Type definitions for CP

import { formatU64, formatI64 } from "../../itoa/src/index.cp";
export class PinoField {
    key: string,
    kind: u8,
    str_val: string,
    int_val: i64,
    bool_val: bool,
}

export fn pinoPush(out: []u8, pos: usize, s: string): usize;
export fn pinoPushJsonStr(out: []u8, pos: usize, s: string): usize;
export fn pinoPushU64(out: []u8, pos: usize, v: u64): usize;
export fn pinoPushI64(out: []u8, pos: usize, v: i64): usize;
export fn levelLabel(level: u8): string;
export class PinoLogger {
    min_level: u8,
    name: string,
    pid: u32,
    hostname: string,
    bindings: [16]PinoField,
    binding_count: usize,
    fn new(min_level: u8, name: string, pid: u32, hostname: string): PinoLogger;
    fn isLevelEnabled(level: u8): bool;
    fn bindStr(_: auto, key: string, val: string): bool;
    fn bindInt(_: auto, key: string, val: i64): bool;
    fn bindBool(_: auto, key: string, val: bool): bool;
    fn formatLine(level: u8, time_ms: u64, msg: string, out: []u8): usize;
}

