// Type definitions for CP

export class SourceEdit {
    start: usize,
    end: usize,
    replacement: string,
}

export fn wsrcWriteStr(out: []u8, pos: usize, s: string): usize;
export fn wsrcEmptyEdit(): SourceEdit;
export class ReplaceSource {
    original: string,
    edits: [64]SourceEdit,
    edit_count: usize,
    fn init(original: string): ReplaceSource;
    fn replace(_: auto, start: usize, end: usize, replacement: string): bool;
    fn insert(_: auto, pos: usize, text: string): bool;
    fn remove(_: auto, start: usize, end: usize): bool;
    fn render(out: []u8): usize;
}

export fn concatSources(parts: &[]string, sep: string, out: []u8): usize;
