// Type definitions for CP

import { isAlnum, isDigit, isSpace } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";
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
export fn isWsCp(cp: u32): bool;
export fn isSentenceEnd(cp: u32): bool;
export fn analyzeTextWithWpm(text: string, en_wpm: usize, cjk_cpm: usize): TextStats;
export fn analyzeText(text: string): TextStats;
export fn formatReadingTime(stats: TextStats, zh: bool, out: []u8): string;
