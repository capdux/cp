// Type definitions for CP

import { isSpace, trim, strEq, rfindByte } from "cap:ascii";
import { parseU32 } from "cap:fmt";
export class StackFrame {
    function_name: string,
    file: string,
    line: u32,
    column: u32,
    is_native: bool,
    valid: bool,
}

export fn stkuInvalid(): StackFrame;
export fn stkuParseLoc(fn_name: string, loc: string): StackFrame;
export fn parseStackLine(line: string): StackFrame;
