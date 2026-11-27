// Type definitions for CP

export class JsToken {
    kind: u8,
    start: usize,
    end: usize,
    line: usize,
    col: usize,
}

export class JsModuleSummary {
    ok: bool,
    token_count: usize,
    import_count: usize,
    export_count: usize,
    function_count: usize,
    class_count: usize,
    unmatched_bracket_pos: usize,
}

export fn acnStrEq(a: string, b: string): bool;
export fn acnIsSpace(ch: u8): bool;
export fn acnIsDigit(ch: u8): bool;
export fn acnIsHexDigit(ch: u8): bool;
export fn acnIsBinDigit(ch: u8): bool;
export fn acnIsOctDigit(ch: u8): bool;
export fn acnIsIdentStart(ch: u8): bool;
export fn acnIsIdentPart(ch: u8): bool;
export fn acnIsKeyword(s: string): bool;
export fn acnMatch3(src: string, pos: usize, a: u8, b: u8, c: u8): bool;
export fn acnMatch2(src: string, pos: usize, a: u8, b: u8): bool;
export fn acnPunctLen(src: string, pos: usize): usize;
export class JsLexer {
    src: string,
    pos: usize,
    line: usize,
    col: usize,
    fn new(src: string): JsLexer;
    fn advance(_: auto);
    fn skipWhitespace(_: auto);
    fn nextToken(_: auto): JsToken;
}

export fn acnMatchingOpen(close_ch: u8): u8;
export fn analyzeJsModule(src: string): JsModuleSummary;
export fn extractImportSpecifiers(src: string, out_specs: []string): usize;
