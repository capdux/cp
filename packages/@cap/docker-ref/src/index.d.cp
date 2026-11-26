// Type definitions for CP

export class ImageRef {
    registry: string,
    repository: string,
    tag: string,
    digest: string,
    valid: bool,
}

export fn drIsLower(c: u8): bool;
export fn drIsUpper(c: u8): bool;
export fn drIsDigit(c: u8): bool;
export fn drIsLowerHex(c: u8): bool;
export fn drEqBytes(a: &[]u8, b: &[]u8): bool;
export fn drContainsByte(s: &[]u8, target: u8): bool;
export fn isValidTag(tag: string): bool;
export fn isValidDigest(digest: string): bool;
export fn drIsValidRegistry(reg: &[]u8): bool;
export fn drIsValidRepository(repo: &[]u8): bool;
export fn parseImageRef(input: string): ImageRef;
export fn drWrite(out: []u8, pos: &mut usize, src: &[]u8): bool;
export fn formatNormalized(ref_in: ImageRef, out: []u8): usize;
