// Type definitions for CP

export class DidUrl {
    did: string,
    method: string,
    id: string,
    path: string,
    query: string,
    fragment: string,
    valid: bool,
}

export fn bytesEq(a: string, b: string): bool;
export fn isHex(c: u8): bool;
export fn isValidMethod(method: string): bool;
export fn isValidMethodId(id: string): bool;
export fn parseDid(input: string): DidUrl;
export fn appendSlice(out: []u8, pos: usize, s: string): i64;
export fn formatDid(method: string, id: string, fragment: string, out: []u8): usize;
