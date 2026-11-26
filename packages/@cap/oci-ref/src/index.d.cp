// Type definitions for CP

export class OciRef {
    valid: bool,
    registry: string,
    repository: string,
    tag: string,
    digest: string,
    is_official_library: bool,
}

export fn findByte(s: string, ch: u8): usize?;
export fn findLastByte(s: string, ch: u8): usize?;
export fn isValidDigest(d: string): bool;
export fn isValidTag(t: string): bool;
export fn isValidRepoPath(p: string): bool;
export fn isRegistryDomain(seg: string): bool;
export fn parseOciRef(raw: string): OciRef;
export fn formatCanonical(r: OciRef, out: []u8): string;
export fn isPinned(r: OciRef): bool;
