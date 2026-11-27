// Type definitions for CP

import { writeStr, writeU64 } from "cap:fmt";
export class Point {
    line: u32,
    column: u32,
    offset: u32,
}

export class Position {
    start: Point,
    end: Point,
}

export enum Severity {
    Info,
    Warning,
    Error
}

export class VFileMessage {
    reason: string,
    source: string,
    rule_id: string,
    file_path: string,
    severity: Severity,
    position: Position,
    fn isFatal(): bool;
    fn hasPosition(): bool;
}

export fn makePoint(line: u32, column: u32, offset: u32): Point;
export fn makePosition(start: Point, end: Point): Position;
export fn pointAtOffset(text: string, offset: usize): Point;
export fn createMessage(reason: string, file_path: string, source: string, rule_id: string, severity: Severity, at: Point): VFileMessage;
export fn stringifyPosition(pos: Position, out: []u8): usize;
export fn formatMessage(msg: VFileMessage, out: []u8): usize;
