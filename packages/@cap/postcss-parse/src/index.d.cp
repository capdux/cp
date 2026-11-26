// Type definitions for CP

export fn pcIsSpace(c: u8): bool;
export fn pcToLower(c: u8): u8;
export fn pcStrEq(a: &[]u8, b: string): bool;
export fn pcStrEqIgnoreCase(a: &[]u8, b: string): bool;
export fn pcCopyTrimmed(src: &[]u8, dst: []u8): usize;
export fn pcAppendSlice(out: []u8, pos: &mut usize, src: &[]u8);
export fn pcAppendByte(out: []u8, pos: &mut usize, b: u8);
export fn pcIsDeclAtRule(name: &[]u8): bool;
export class PostCssDecl {
    kind: u8,
    parent_node: i32,
    prop: [48]u8,
    prop_len: usize,
    value: [128]u8,
    value_len: usize,
    important: bool,
    fn empty(): PostCssDecl;
    fn propStr(): &[]u8;
    fn valueStr(): &[]u8;
}

export class PostCssNode {
    kind: u8,
    parent_node: i32,
    name: [64]u8,
    name_len: usize,
    params: [128]u8,
    params_len: usize,
    has_block: bool,
    fn empty(): PostCssNode;
    fn nameStr(): &[]u8;
    fn paramsStr(): &[]u8;
}

export class PostCssComment {
    kind: u8,
    text: [128]u8,
    text_len: usize,
    fn empty(): PostCssComment;
    fn textStr(): &[]u8;
}

export fn pcAddComment(sheet: &mut PostCssSheet, raw_text: &[]u8);
export fn pcSkipWsAndComments(css: string, pos: &mut usize, end: usize, sheet: &mut PostCssSheet): bool;
export fn pcScanString(css: string, pos: &mut usize, end: usize, sheet: &mut PostCssSheet): bool;
export fn pcExtractCleanText(css: string, start: usize, end: usize, dst: []u8, sheet: &mut PostCssSheet): usize;
export fn pcStripImportant(buf: []u8, len: &mut usize): bool;
export fn pcParseDecls(css: string, start: usize, end: usize, parent_node: i32, sheet: &mut PostCssSheet);
export fn pcFindMatchingBrace(css: string, open_pos: usize, end: usize, out_close: &mut usize, sheet: &mut PostCssSheet): bool;
export fn pcParseBlockList(css: string, start: usize, end: usize, parent_node: i32, depth: usize, sheet: &mut PostCssSheet);
export fn pcSerializeNode(sheet: PostCssSheet, node_idx: usize, indent: usize, out: []u8, pos: &mut usize);
export class PostCssSheet {
    nodes: [32]PostCssNode,
    node_len: usize,
    decls: [64]PostCssDecl,
    decl_len: usize,
    comments: [16]PostCssComment,
    comment_len: usize,
    ok: bool,
    fn empty(): PostCssSheet;
    fn parse(css: string): PostCssSheet;
    fn ruleCount(): usize;
    fn declCount(): usize;
    fn atRuleCount(): usize;
    fn commentCount(): usize;
    fn findDecl(selector: string, prop: string, out: []u8): i64;
    fn isDeclImportant(selector: string, prop: string): bool;
    fn setDecl(_: auto, selector: string, prop: string, val: string, important: bool): bool;
    fn toCss(out: []u8): usize;
}

