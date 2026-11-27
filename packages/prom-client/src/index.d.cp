// Type definitions for CP

import { isAlpha, isAlnum } from "cap:ascii";
import { writeStr, writeI64 } from "cap:fmt";
export class Label {
    key: &[]u8,
    val: &[]u8,
}

export fn isValidMetricName(name: &[]u8): bool;
export fn isValidLabelName(name: &[]u8): bool;
export class PromWriter {
    buf: []u8,
    pos: usize,
    overflow: bool,
    fn init(buf: []u8): PromWriter;
    fn writeByte(_: auto, b: u8): bool;
    fn writeSlice(_: auto, s: &[]u8): bool;
    fn writeInt(_: auto, v: i64): bool;
    fn writeEscapedHelp(_: auto, help: &[]u8): bool;
    fn writeEscapedLabelVal(_: auto, val: &[]u8): bool;
    fn writeHelp(_: auto, name: &[]u8, help: &[]u8): bool;
    fn writeType(_: auto, name: &[]u8, kind: u8): bool;
    fn writeSamplePrefix(_: auto, name: &[]u8, labels: &[]Label): bool;
    fn writeSampleInt(_: auto, name: &[]u8, labels: &[]Label, value: i64): bool;
    fn writeSampleWithTs(_: auto, name: &[]u8, labels: &[]Label, value: i64, ts_ms: i64): bool;
    fn written(): &[]u8;
    fn len(): usize;
    fn isOverflow(): bool;
}

