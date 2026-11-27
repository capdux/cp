// @cap/vfile-message — Structured lint/AST diagnostic messages & unist positions in pure CP
// Clean-room MIT implementation of npm `vfile-message` (#556, 102.9M/wk) & `unist-util-position` (#669).

import { writeStr, writeU64 } from "cap:fmt";

pub struct Point {
    pub line: u32,   // 1-based line (0 = unset)
    pub column: u32, // 1-based column (0 = unset)
    pub offset: u32, // 0-based byte offset
}

pub struct Position {
    pub start: Point,
    pub end: Point,
}

pub enum Severity {
    Info,
    Warning,
    Error,
}

pub struct VFileMessage {
    pub reason: string,
    pub source: string,
    pub rule_id: string,
    pub file_path: string,
    pub severity: Severity,
    pub position: Position,

    pub fn isFatal(): bool {
        return _.severity == .Error;
    }

    pub fn hasPosition(): bool {
        return _.position.start.line > 0 && _.position.start.column > 0;
    }
}

/// Creates a `Point` (1-based line and column, 0-based byte offset).
pub fn makePoint(line: u32, column: u32, offset: u32): Point {
    return Point { line: line, column: column, offset: offset };
}

/// Creates a `Position` spanning from `start` to `end`.
pub fn makePosition(start: Point, end: Point): Position {
    return Position { start: start, end: end };
}

/// Computes 1-based `(line, column)` `Point` from a byte `offset` in UTF-8 text.
pub fn pointAtOffset(text: string, offset: usize): Point {
    let line: u32 = 1;
    let col: u32 = 1;
    let limit = offset;
    if (limit > text.len) {
        limit = text.len;
    }
    let i: usize = 0;
    while (i < limit) {
        if (text[i] == 10) { // '\n'
            line += 1;
            col = 1;
        } else {
            col += 1;
        }
        i += 1;
    }
    return Point { line: line, column: col, offset: limit as u32 };
}

/// Creates a `VFileMessage` at a single `Point` (sets `end = start`).
pub fn createMessage(
    reason: string,
    file_path: string,
    source: string,
    rule_id: string,
    severity: Severity,
    at: Point,
): VFileMessage {
    return VFileMessage {
        reason: reason,
        source: source,
        rule_id: rule_id,
        file_path: file_path,
        severity: severity,
        position: Position { start: at, end: at },
    };
}

/// Formats a `Position` as `L:C` or `L1:C1-L2:C2` into `out`; returns bytes written.
pub fn stringifyPosition(pos: Position, out: []u8): usize {
    if (pos.start.line == 0 || pos.start.column == 0) {
        return 0;
    }
    let p: usize = 0;
    p = writeU64(pos.start.line as u64, out, p);
    p = writeStr(":", out, p);
    p = writeU64(pos.start.column as u64, out, p);
    if (pos.end.line > 0 && pos.end.column > 0 &&
        (pos.end.line != pos.start.line || pos.end.column != pos.start.column)) {
        p = writeStr("-", out, p);
        p = writeU64(pos.end.line as u64, out, p);
        p = writeStr(":", out, p);
        p = writeU64(pos.end.column as u64, out, p);
    }
    return p;
}

/// Formats a `VFileMessage` as `[path:]L:C[-L2:C2]: severity: reason [source:rule_id]`.
pub fn formatMessage(msg: VFileMessage, out: []u8): usize {
    let p: usize = 0;
    if (msg.file_path.len > 0) {
        p = writeStr(msg.file_path, out, p);
        if (msg.hasPosition()) {
            p = writeStr(":", out, p);
        }
    }
    if (msg.hasPosition()) {
        const written = stringifyPosition(msg.position, out[p..out.len]);
        p += written;
    }
    if (p > 0) {
        p = writeStr(": ", out, p);
    }
    if (msg.severity == .Error) {
        p = writeStr("error: ", out, p);
    } else if (msg.severity == .Warning) {
        p = writeStr("warning: ", out, p);
    } else {
        p = writeStr("info: ", out, p);
    }
    p = writeStr(msg.reason, out, p);
    if (msg.source.len > 0 || msg.rule_id.len > 0) {
        p = writeStr(" [", out, p);
        if (msg.source.len > 0) {
            p = writeStr(msg.source, out, p);
            if (msg.rule_id.len > 0) {
                p = writeStr(":", out, p);
            }
        }
        if (msg.rule_id.len > 0) {
            p = writeStr(msg.rule_id, out, p);
        }
        p = writeStr("]", out, p);
    }
    return p;
}

export {
    Point,
    Position,
    Severity,
    VFileMessage,
    makePoint,
    makePosition,
    pointAtOffset,
    createMessage,
    stringifyPosition,
    formatMessage,
};
