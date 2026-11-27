// Type definitions for CP

export class Bcp47Tag {
    language: string,
    script: string,
    region: string,
    variant: string,
    valid: bool,
}

export fn ltgIsAlpha(c: u8): bool;
export fn ltgIsDigit(c: u8): bool;
export fn ltgIsAlnum(c: u8): bool;
export fn ltgIsSep(c: u8): bool;
export fn ltgToLower(c: u8): u8;
export fn ltgToUpper(c: u8): u8;
export fn ltgAllAlpha(s: string): bool;
export fn ltgAllDigit(s: string): bool;
export fn ltgAllAlnum(s: string): bool;
export fn ltgInvalidTag(): Bcp47Tag;
export fn parseLanguageTag(tag: string): Bcp47Tag;
export fn ltgPushByte(out: []u8, pos: usize, b: u8): usize;
export fn formatCanonicalTag(t: Bcp47Tag, out: []u8): usize;
export fn matchesLanguageRange(tag: string, range: string): bool;
