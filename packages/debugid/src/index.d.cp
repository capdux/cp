// Type definitions for CP

export class DebugId {
    bytes: [16]u8,
    appendix: u32,
    valid: bool,
    fn isNil(): bool;
}

export fn dbgHexVal(b: u8): u8?;
export fn dbgHexChar(n: u8, upper: bool): u8;
export fn parseDebugId(s: string): DebugId;
export fn dbgWriteHexByte(b: u8, upper: bool, out: []u8, pos: usize): usize;
export fn dbgWriteHexU32(val: u32, upper: bool, out: []u8, pos: usize): usize;
export fn formatBreakpad(id: DebugId, out: []u8): usize;
export fn formatCanonical(id: DebugId, out: []u8): usize;
