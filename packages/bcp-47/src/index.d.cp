// Type definitions for CP

import { isAlpha, isDigit, isAlnum, toLowerByte, toUpperByte, trim, eqIgnoreCase } from "cap:ascii";
export class Bcp47Tag {
    valid: bool,
    language: string,
    script: string,
    region: string,
    variant: string,
    extension: string,
    private_use: string,
    fn invalid(): Bcp47Tag;
}

export class SubtagSpan {
    start: usize,
    end: usize,
}

export fn eqSubtagNormalized(a: string, b: string): bool;
export fn isAllAlpha(s: string): bool;
export fn isAllDigit(s: string): bool;
export fn isAllAlphaNum(s: string): bool;
export fn isVariantSubtag(s: string): bool;
export fn isSingletonSubtag(s: string): bool;
export fn isPrivateIntro(s: string): bool;
export fn parseBcp47(raw: string): Bcp47Tag;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn appendLowerHyphen(out: []u8, start_pos: usize, s: string): usize;
export fn appendUpper(out: []u8, start_pos: usize, s: string): usize;
export fn appendTitle(out: []u8, start_pos: usize, s: string): usize;
export fn canonicalizeBcp47(tag: Bcp47Tag, out: []u8): string;
export fn scoreLocaleMatch(req: Bcp47Tag, cand: Bcp47Tag): u32;
export fn matchLocale(requested: Bcp47Tag, supported: &[]string): string?;
