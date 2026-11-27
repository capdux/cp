// Type definitions for CP

export enum BreakKind {
    Mandatory,
    Allowed
}

export class LineBreak {
    offset: usize,
    kind: BreakKind,
}

export class UlbrDecoded {
    cp: u32,
    len: usize,
}

export fn ulbrDecodeUtf8(s: string, pos: usize): UlbrDecoded;
export fn ulbrClassify(cp: u32): u8;
export fn ulbrEvalBreak(prev: u8, prevNonSp: u8, curr: u8): u8;
export fn findLineBreaks(utf8: string, out: []LineBreak): usize;
export fn countLineBreaks(utf8: string): usize;
