// Type definitions for CP

export fn exsIsHspace(c: u8): bool;
export fn exsIsFrameLine(line: string): bool;
export fn exsFindFirstFrameOffset(stack: string): usize;
export fn exsTrimLine(line: string): string;
export fn extractStack(stack: string): string;
export fn extractErrorHeader(stack: string): string;
export fn extractStackLines(stack: string, out: []string): usize;
export class StackFrame {
    function_name: string,
    file: string,
    line: u32,
    column: u32,
    is_native: bool,
    is_async: bool,
    format: u8,
    valid: bool,
}

export fn exsEmptyFrame(): StackFrame;
export fn exsParseU32(s: string): u32;
export fn exsIsAllDigits(s: string): bool;
export fn exsFindLastByte(s: string, ch: u8): i32;
export fn exsStartsWith(s: string, prefix: string): bool;
export fn exsStrEq(a: string, b: string): bool;
export fn exsFindSubstr(s: string, needle: string): i32;
export fn exsParseLocation(loc: string, frame: &mut StackFrame): bool;
export fn parseStackFrame(frame_line: string): StackFrame;
export fn parseStackFrames(stack: string, out: []StackFrame): usize;
export fn countStackFrames(stack: string): usize;
