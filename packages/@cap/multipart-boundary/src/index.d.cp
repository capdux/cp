// Type definitions for CP

export class MultipartPart {
    headers: string,
    body: string,
    name: string,
    filename: string,
    content_type: string,
    next_pos: usize,
    valid: bool,
}

export fn isWs(c: u8): bool;
export fn toLowerAscii(c: u8): u8;
export fn eqIgnoreCase(a: string, b: string): bool;
export fn bytesEq(a: string, b: string): bool;
export fn isBchar(c: u8): bool;
export fn isValidBoundary(boundary: string): bool;
export fn extractBoundary(content_type: string): string;
export fn matchBoundaryAt(payload: string, pos: usize, boundary: string): bool;
export fn extractDispositionParam(disp: string, key: string): string;
export fn nextPart(payload: string, boundary: string, start: usize): MultipartPart;
export fn countParts(payload: string, boundary: string): usize;
