// Type definitions for CP

export enum DetectedCharset {
    Utf8,
    Utf8Bom,
    Utf16Le,
    Utf16Be,
    Utf32Le,
    Utf32Be,
    Ascii,
    Windows1252,
    Iso8859_1,
    ShiftJis,
    Gbk,
    Unknown
}

export class CharsetMatch {
    charset: DetectedCharset,
    name: string,
    confidence: u8,
}

export fn charsetName(cs: DetectedCharset): string;
export fn chdCheckUtf8(data: &[]u8): u8;
export fn chdScoreShiftJis(data: &[]u8): u8;
export fn chdScoreGbk(data: &[]u8): u8;
export fn detect(data: &[]u8): CharsetMatch;
export fn detectName(data: &[]u8): string;
