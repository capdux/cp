// Type definitions for CP

import { eqIgnoreCase } from "cap:ascii";
import { bytesEq, findByteInBytes } from "cap:bin";
import { writeStr, parseI64 } from "cap:fmt";
export class IniEntry {
    section_start: usize,
    section_len: usize,
    key_start: usize,
    key_len: usize,
    val_start: usize,
    val_len: usize,
}

export fn isWs(c: u8): bool;
export class IniDoc {
    entries: [128]IniEntry,
    count: usize,
    fn parse(src: &[]u8): IniDoc;
    fn len(): usize;
    fn findIndex(src: &[]u8, section: &[]u8, key: &[]u8): i64;
    fn has(src: &[]u8, section: &[]u8, key: &[]u8): bool;
    fn get(src: &[]u8, section: &[]u8, key: &[]u8): &[]u8;
    fn getBool(src: &[]u8, section: &[]u8, key: &[]u8, default_val: bool): bool;
    fn getInt(src: &[]u8, section: &[]u8, key: &[]u8, default_val: i64): i64;
}

export class IniWriter {
    buf: []u8,
    pos: usize,
    overflow: bool,
    fn init(buf: []u8): IniWriter;
    fn writeByte(_: auto, b: u8): bool;
    fn writeSlice(_: auto, s: &[]u8): bool;
    fn writeSection(_: auto, name: &[]u8): bool;
    fn writeEntry(_: auto, key: &[]u8, value: &[]u8): bool;
    fn written(): &[]u8;
    fn len(): usize;
}

