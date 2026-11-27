// Type definitions for CP

export enum Script {
    Common,
    Latin,
    Greek,
    Cyrillic,
    Han,
    Hiragana,
    Katakana,
    Hangul,
    Arabic,
    Hebrew,
    Devanagari,
    Thai,
    Inherited,
    Unknown
}

export fn uscScriptIndex(s: Script): usize;
export fn uscIndexToScript(idx: usize): Script;
export fn scriptOf(cp: u32): Script;
export fn scriptName(s: Script): string;
export fn isLatin(cp: u32): bool;
export fn isHan(cp: u32): bool;
export fn isCjkScript(s: Script): bool;
export class UscDecoded {
    cp: u32,
    len: usize,
}

export fn uscDecodeUtf8(text: string, pos: usize): UscDecoded;
export fn dominantScript(text: string): Script;
