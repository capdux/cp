// Type definitions for CP

import { trim, strEq, startsWith, findByte, rfindByte } from "cap:ascii";
import { writeStr, writeU64, parseU32 } from "cap:fmt";
import { indexOfBytes } from "cap:bin";
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
export fn formatFrameAt(f: StackFrame, out: []u8, start_pos: usize): usize;
export fn formatFrame(f: StackFrame, out: []u8): string;
export fn cleanStack(raw: string, out: []u8): string;
