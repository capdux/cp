// Type definitions for CP

export error TemplateError { BufferTooSmall, UnclosedTag, UnclosedBlock, MismatchedBlock, DepthExceeded, ContextFull, UnknownFilter }

export class VarEntry {
    key: &[]u8,
    val: &[]u8,
    in_pool: bool,
    pool_off: u16,
    pool_len: u16,
}

export class ListMeta {
    key: &[]u8,
    count: usize,
}

export class ListItemEntry {
    list_idx: u8,
    val: &[]u8,
}

export enum BlockKind {
    If,
    Unless,
    Each
}

export class BlockFrame {
    kind: BlockKind,
    parent_active: bool,
    active: bool,
    has_else: bool,
    each_body_start: usize,
    each_list_key: &[]u8,
    each_index: usize,
    each_len: usize,
}

export fn isSpace(c: u8): bool;
export fn trimSlice(s: &[]u8): &[]u8;
export fn isValueTruthy(val: &[]u8): bool;
export fn formatUsize(n: usize, buf: []u8): &[]u8;
export fn formatI64(n: i64, buf: []u8): &[]u8;
export fn appendBytes(out: []u8, pos: &mut usize, data: &[]u8): void ! TemplateError;
export fn appendEscapedHtml(out: []u8, pos: &mut usize, input: &[]u8): void ! TemplateError;
export fn escapeHtml(input: &[]u8, out: []u8): &[]u8 ! TemplateError;
export class TemplateContext {
    vars: [64]VarEntry,
    var_count: usize,
    lists: [16]ListMeta,
    list_count: usize,
    items: [128]ListItemEntry,
    item_count: usize,
    int_pool: [512]u8,
    int_pool_len: usize,
    fn init(): TemplateContext;
    fn reset(_: auto);
    fn set(_: auto, key: &[]u8, val: &[]u8): void ! TemplateError;
    fn setBool(_: auto, key: &[]u8, b: bool): void ! TemplateError;
    fn setInt(_: auto, key: &[]u8, n: i64): void ! TemplateError;
    fn get(key: &[]u8): (&[]u8)?;
    fn has(key: &[]u8): bool;
    fn varCount(): usize;
    fn pushListItem(_: auto, list_key: &[]u8, item: &[]u8): void ! TemplateError;
    fn listLen(list_key: &[]u8): usize;
    fn listItem(list_key: &[]u8, idx: usize): (&[]u8)?;
    fn isTruthy(key: &[]u8): bool;
}

export fn findActiveEach(stack: &[16]BlockFrame, depth: usize): usize?;
export fn resolveValue(key: &[]u8, ctx: &TemplateContext, stack: &[16]BlockFrame, depth: usize, index_buf: []u8): &[]u8;
export fn resolveCondition(key: &[]u8, ctx: &TemplateContext, stack: &[16]BlockFrame, depth: usize): bool;
export fn applyFilter(filter_name: &[]u8, input: &[]u8, dst: []u8): &[]u8 ! TemplateError;
export fn evalExpressionAndEmit(expr: &[]u8, raw_output: bool, ctx: &TemplateContext, stack: &[16]BlockFrame, depth: usize, out: []u8, out_pos: &mut usize): void ! TemplateError;
export fn findSubstring(haystack: &[]u8, start: usize, needle: &[]u8): usize?;
export fn isBranchActive(stack: &[16]BlockFrame, depth: usize): bool;
export fn render(tpl: &[]u8, ctx: &TemplateContext, out: []u8): &[]u8 ! TemplateError;
