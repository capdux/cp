// Type definitions for CP

export class MinifyOptions {
    strip_comments: bool,
    preserve_legal_comments: bool,
    collapse_whitespace: bool,
    keep_newlines: bool,
    fn defaults(): MinifyOptions;
}

export class MinifyStats {
    input_len: usize,
    output_len: usize,
    comments_removed: usize,
    ok: bool,
}

export fn tmIsSpace(ch: u8): bool;
export fn tmIsWord(ch: u8): bool;
export fn tmNeedsJsSpace(prev: u8, next: u8): bool;
export fn tmContainsSubstr(s: string, needle: string): bool;
export fn tmIsLegalComment(comment: string): bool;
export fn tmIsCssPunct(ch: u8): bool;
export fn minifyJs(input: string, opts: MinifyOptions, out: []u8): MinifyStats;
export fn minifyCss(input: string, out: []u8): MinifyStats;
