// Type definitions for CP

export class TextCounts {
    bytes: usize,
    chars: usize,
    words: usize,
    cjk_chars: usize,
    latin_words: usize,
    lines: usize,
}

export fn isCjkCodepoint(cp: u32): bool;
export fn wcIsLatinWordChar(cp: u32): bool;
export fn wcNextCodepoint(text: &[]u8, pos: usize, next_pos: &mut usize): u32;
export fn analyze(text: string): TextCounts;
export fn countWords(text: string): usize;
export fn estimateReadingSeconds(text: string, words_per_minute: u32): u32;
