// Type definitions for CP

export class StackFrame {
    func_name: string,
    file: string,
    line: u32,
    column: u32,
    is_native: bool,
    is_internal: bool,
    fn empty(): StackFrame;
}

export class ParsedStack {
    frames: [MAX_FRAMES]StackFrame,
    count: usize,
    message: string,
    fn empty(): ParsedStack;
    fn frameAt(i: usize): StackFrame;
}

export fn isSpace(c: u8): bool;
export fn isDigit(c: u8): bool;
export fn trim(s: string): string;
export fn strEq(a: string, b: string): bool;
export fn startsWith(s: string, prefix: string): bool;
export fn parseU32Strict(s: string): u32?;
export fn containsChar(s: string, ch: u8): bool;
export fn findSubstring(hay: string, needle: string): usize;
export fn isInternalLocation(file: string, is_native: bool): bool;
export class LocationInfo {
    file: string,
    line: u32,
    column: u32,
    is_native: bool,
}

export fn parseLocation(raw_loc: string): LocationInfo;
export fn parseFrameLine(raw_line: string): StackFrame?;
export fn parseStack(raw: string): ParsedStack;
export fn pushByte(out: []u8, pos: usize, b: u8): usize;
export fn pushStr(out: []u8, pos: usize, s: string): usize;
export fn pushU32(out: []u8, pos: usize, val: u32): usize;
export fn formatFrameAt(f: StackFrame, out: []u8, start_pos: usize): usize;
export fn formatFrame(f: StackFrame, out: []u8): string;
export fn cleanStack(raw: string, out: []u8): string;
