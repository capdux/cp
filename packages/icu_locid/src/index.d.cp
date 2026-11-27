// Type definitions for CP

import { TinyAsciiStr4, TinyAsciiStr8 } from "../../tinystr/src/index.cp";
export fn locIsSep(ch: u8): bool;
export fn locIsDigitByte(ch: u8): bool;
export fn locEmptyVariants(): [4]TinyAsciiStr8;
export fn locLangEqStr(lang: TinyAsciiStr8, lit: string): bool;
export fn locRegionEqStr(reg: TinyAsciiStr4, lit: string): bool;
export fn locValidLanguageSubtag(sub: TinyAsciiStr8): bool;
export fn locValidVariantSubtag(sub: TinyAsciiStr8): bool;
export class LanguageIdentifier {
    language: TinyAsciiStr8,
    has_script: bool,
    script: TinyAsciiStr4,
    has_region: bool,
    region: TinyAsciiStr4,
    variant_count: usize,
    variants: [4]TinyAsciiStr8,
    fn und(): LanguageIdentifier;
    fn parse(input: string): LocaleParseResult;
    fn toStr(out: []u8): usize;
    fn isUnd(): bool;
    fn eq(other: LanguageIdentifier): bool;
    fn matches(other: LanguageIdentifier): bool;
    fn maximizeLikelySubtags(): LanguageIdentifier;
}

export class LocaleParseResult {
    ok: bool,
    locale: LanguageIdentifier,
    err: i64,
}

