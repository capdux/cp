// Type definitions for CP

export class StackFrame {
    function_name: string,
    file: string,
    line: u32,
    column: u32,
    is_native: bool,
    valid: bool,
}

export fn stkuIsSpace(ch: u8): bool;
export fn stkuTrim(s: string): string;
export fn stkuEq(a: string, b: string): bool;
export fn stkuIsDigits(s: string): bool;
export fn stkuParseU32(s: string): u32;
export fn stkuLastColon(s: string): usize;
export fn stkuInvalid(): StackFrame;
export fn stkuParseLoc(fn_name: string, loc: string): StackFrame;
export fn parseStackLine(line: string): StackFrame;
