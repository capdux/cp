// Type definitions for CP

import { strEq } from "cap:ascii";
import { writeStr, writeU64, writeI64 } from "cap:fmt";
export fn pfmtWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn pfmtWriteSpaces(out: []u8, pos: usize, count: usize): usize;
export fn pfmtWriteF64(out: []u8, pos: usize, val: f64): usize;
export fn pfmtWriteStringLiteral(out: []u8, pos: usize, s: string, escape_string: bool): usize;
export class PrettyOptions {
    indent: usize,
    max_depth: usize,
    min: bool,
    escape_string: bool,
    print_basic_prototype: bool,
    fn defaults(): PrettyOptions;
    fn compact(): PrettyOptions;
}

export class PrettyWriter {
    opts: PrettyOptions,
    buf: [2048]u8,
    len: usize,
    depth: usize,
    item_counts: [32]usize,
    truncated_depth: usize,
    fn new(opts: PrettyOptions): PrettyWriter;
    fn defaultWriter(): PrettyWriter;
    fn clear(_: auto);
    fn isTruncated(): bool;
    fn writeNull(_: auto);
    fn writeUndefined(_: auto);
    fn writeBool(_: auto, val: bool);
    fn writeInt(_: auto, val: i64);
    fn writeFloat(_: auto, val: f64);
    fn writeString(_: auto, s: string);
    fn writeSymbol(_: auto, desc: string);
    fn writeFunction(_: auto, name: string);
    fn beginObject(_: auto, type_name: string): bool;
    fn writeKey(_: auto, k: string);
    fn endObject(_: auto);
    fn beginArray(_: auto, type_name: string): bool;
    fn nextItem(_: auto);
    fn endArray(_: auto);
}

export fn formatStringLiteral(s: string, escape: bool, out: []u8): usize;
export fn formatPrimitiveInt(v: i64, out: []u8): usize;
