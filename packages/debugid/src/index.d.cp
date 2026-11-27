// Type definitions for CP

import { hexVal } from "cap:ascii";
import { BufWriter } from "cap:fmt";
export class DebugId {
    bytes: [16]u8,
    appendix: u32,
    valid: bool,
    fn isNil(): bool;
}

export fn parseDebugId(s: string): DebugId;
export fn formatBreakpad(id: DebugId, out: []u8): usize;
export fn formatCanonical(id: DebugId, out: []u8): usize;
