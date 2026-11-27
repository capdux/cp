// Type definitions for CP

import { isSpace } from "cap:ascii";
import { bytesEq } from "cap:bin";
import { writeStr } from "cap:fmt";
export class CondClass {
    name: &[]u8,
    enabled: bool,
}

export class ClassBuilder {
    buf: []u8,
    pos: usize,
    overflow: bool,
    fn init(buf: []u8): ClassBuilder;
    fn reset(_: auto);
    fn has(token: &[]u8): bool;
    fn appendToken(_: auto, tok: &[]u8): bool;
    fn add(_: auto, cls: &[]u8): bool;
    fn addIf(_: auto, cond: bool, cls: &[]u8): bool;
    fn addUnique(_: auto, cls: &[]u8): bool;
    fn addConds(_: auto, items: &[]CondClass): bool;
    fn build(): &[]u8;
    fn len(): usize;
    fn isOverflow(): bool;
}

export fn joinClasses(classes: &[]&[]u8, dst: []u8): i64;
export fn joinCondClasses(items: &[]CondClass, dst: []u8): i64;
