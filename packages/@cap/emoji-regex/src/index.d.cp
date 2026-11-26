// Type definitions for CP

export class EmojiSpan {
    start: usize,
    end: usize,
}

export class EmjDecoded {
    cp: u32,
    len: usize,
}

export fn emjDecodeUtf8(s: string, pos: usize): EmjDecoded;
export fn emjIsRegionalIndicator(cp: u32): bool;
export fn emjIsSkinTone(cp: u32): bool;
export fn emjIsKeycapBase(cp: u32): bool;
export fn emjIsTagChar(cp: u32): bool;
export fn isEmojiCodePoint(cp: u32): bool;
export fn emjConsumeModifiers(s: string, start_pos: usize): usize;
export fn findNextEmoji(s: string, from_pos: usize): EmojiSpan?;
export fn countEmojis(s: string): usize;
export fn containsEmoji(s: string): bool;
export fn stripEmojis(s: string, out: []u8): usize;
