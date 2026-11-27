// Type definitions for CP

import { isAlnum, eqIgnoreCase, trim, findByte, rfindByte } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class MimeParts {
    valid: bool,
    essence: string,
    type_part: string,
    subtype: string,
    charset_val: string,
}

export fn mtypIsTokenChar(c: u8): bool;
export fn mtypEndsWithIgnoreCase(s: string, suffix: string): bool;
export fn mtypIsValidToken(s: string): bool;
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
