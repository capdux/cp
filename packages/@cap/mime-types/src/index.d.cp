// Type definitions for CP

export class MimeParts {
    valid: bool,
    essence: string,
    type_part: string,
    subtype: string,
    charset_val: string,
}

export fn mtypToLower(c: u8): u8;
export fn mtypIsSpace(c: u8): bool;
export fn mtypIsTokenChar(c: u8): bool;
export fn mtypEqIgnoreCase(a: string, b: string): bool;
export fn mtypEndsWithIgnoreCase(s: string, suffix: string): bool;
export fn mtypContainsSlash(s: string): bool;
export fn mtypTrimBounds(s: string, out_start: &mut usize, out_end: &mut usize);
export fn mtypIsValidToken(s: string): bool;
export fn mtypWriteStr(out: []u8, pos: &mut usize, s: string);
export fn mtypExtractExt(path_or_ext: string): string?;
export fn mtypLookupExt(ext: string): string?;
export fn mtypExtensionForEssence(essence: string): string?;
export fn parseMimeType(raw: string): MimeParts;
export fn lookup(path_or_ext: string): string?;
export fn extension(mime_type: string): string?;
export fn charset(mime_type: string): string?;
export fn mtypIsTopLevelType(t: string): bool;
export fn contentType(type_or_ext: string, out: []u8): usize;
export fn isCompressible(mime_type: string): bool;
