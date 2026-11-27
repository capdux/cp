// Type definitions for CP

export fn isValid(ptr: string): bool;
export fn escapeToken(token: string, out: []u8): usize;
export fn unescapeToken(token: string, out: []u8): usize;
export fn countSegments(ptr: string): usize;
export fn hasSegment(ptr: string, index: usize): bool;
export fn getSegment(ptr: string, index: usize, out: []u8): usize;
export fn parseArrayIndex(token: string): i64;
export fn appendSegment(base: string, token: string, out: []u8): usize;
export fn parentPointer(ptr: string): string;
export class PointerIter {
    ptr: string,
    pos: usize,
    valid: bool,
    fn init(ptr: string): PointerIter;
    fn next(_: auto, out: []u8): i64;
}

