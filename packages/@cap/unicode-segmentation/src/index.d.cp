// Type definitions for CP

export class GraphemeSpan {
    start: usize,
    end: usize,
    fn byteLen(): usize;
}

export class UsegRune {
    cp: u32,
    len: usize,
}

export fn usegDecodeUtf8(src: string, offset: usize): UsegRune;
export fn usegIsExtend(cp: u32): bool;
export fn usegIsPrepend(cp: u32): bool;
export fn usegIsSpacingMark(cp: u32): bool;
export fn usegIsControl(cp: u32): bool;
export fn usegIsExtPic(cp: u32): bool;
export fn usegGraphemeProp(cp: u32): u8;
export fn usegNextGraphemeEnd(src: string, start: usize): usize;
export class GraphemeIter {
    src: string,
    pos: usize,
    fn new(src: string): GraphemeIter;
    fn next(_: auto): GraphemeSpan?;
    fn reset(_: auto);
}

export fn countGraphemes(src: string): usize;
export fn nthGrapheme(src: string, idx: usize): GraphemeSpan?;
export fn sliceGraphemes(src: string, start_idx: usize, max_count: usize): GraphemeSpan;
export fn usegIsCjkWordChar(cp: u32): bool;
export fn usegIsALetter(cp: u32): bool;
export fn usegIsNumeric(cp: u32): bool;
export fn usegWordProp(cp: u32): u8;
export fn usegPeekWordProp(src: string, offset: usize): u8;
export fn usegIsWordSpan(src: string, start: usize, end: usize): bool;
export class WordIter {
    src: string,
    pos: usize,
    fn new(src: string): WordIter;
    fn reset(_: auto);
    fn next(_: auto): GraphemeSpan?;
    fn nextUnicodeWord(_: auto): GraphemeSpan?;
}

export fn countUnicodeWords(src: string): usize;
