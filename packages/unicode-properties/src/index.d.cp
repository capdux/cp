// Type definitions for CP

export fn decodeUtf8Strict(s: string, pos: usize, next_pos: &mut usize): u32;
export fn isXidStart(cp: u32): bool;
export fn isXidContinue(cp: u32): bool;
export fn isUnicodeWhitespace(cp: u32): bool;
export fn isEmojiPresentation(cp: u32): bool;
export fn isEmoji(cp: u32): bool;
export fn isDefaultIgnorable(cp: u32): bool;
export fn isValidIdentifier(s: string): bool;
