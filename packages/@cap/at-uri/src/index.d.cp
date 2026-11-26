// Type definitions for CP

export class AtUri {
    authority: string,
    collection: string,
    rkey: string,
    is_did: bool,
    valid: bool,
}

export fn invalidAtUri(): AtUri;
export fn isLowerAlpha(b: u8): bool;
export fn isAlpha(b: u8): bool;
export fn isDigit(b: u8): bool;
export fn isAlphaNum(b: u8): bool;
export fn isValidNsid(nsid: string): bool;
export fn isValidRkey(rkey: string): bool;
export fn isValidDid(s: string): bool;
export fn isValidHandle(s: string): bool;
export fn parseAtUri(uri: string): AtUri;
export fn pushSlice(dst: []u8, pos: usize, src: string): i64;
export fn formatAtUri(authority: string, collection: string, rkey: string, out: []u8): usize;
