// Type definitions for CP

export class TextStats {
    bytes: usize,
    chars: usize,
    ascii_words: usize,
    cjk_chars: usize,
    total_words: usize,
    lines: usize,
    non_empty_lines: usize,
    paragraphs: usize,
    sentences: usize,
    est_reading_sec: usize,
    est_reading_min: usize,
    est_llm_tokens: usize,
    fn empty(): TextStats;
}

export class DecodedChar {
    cp: u32,
    len: usize,
}

export fn decodeUtf8(s: string, pos: usize): DecodedChar;
export fn isCjkChar(cp: u32): bool;
export fn isWordCoreChar(cp: u32): bool;
export fn isDigitByte(c: u8): bool;
export fn isWsCp(cp: u32): bool;
export fn isSentenceEnd(cp: u32): bool;
export fn analyzeTextWithWpm(text: string, en_wpm: usize, cjk_cpm: usize): TextStats;
export fn analyzeText(text: string): TextStats;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn appendUsize(out: []u8, pos: usize, val: usize): usize;
export fn formatReadingTime(stats: TextStats, zh: bool, out: []u8): string;
