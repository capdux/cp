// Type definitions for CP

import { strEq, isSpace, trim, startsWith, toLowerByte, toUpperByte } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";
import { indexOfBytes } from "cap:bin";
import { escapeHtml } from "../../escape-html/src/index.cp";
export class HbsEntry {
    key: string,
    kind: u8,
    str_val: string,
    bool_val: bool,
    count_val: usize,
}

export class HbsContext {
    entries: [32]HbsEntry,
    len: usize,
    fn new(): HbsContext;
    fn setStr(_: auto, k: string, v: string);
    fn setBool(_: auto, k: string, v: bool);
    fn setCount(_: auto, k: string, n: usize);
    fn size(): usize;
}

export class HbsBlockSpan {
    found: bool,
    has_else: bool,
    else_start: usize,
    else_end: usize,
    close_start: usize,
    close_end: usize,
}

export fn hbsWriteText(out: []u8, pos: usize, s: string, escape: bool): usize;
export fn hbsWriteTransformed(out: []u8, pos: usize, s: string, to_upper: bool, escape: bool): usize;
export fn hbsFindEntry(ctx: HbsContext, key: string): usize;
export fn hbsIsTruthy(ctx: HbsContext, key: string): bool;
export fn hbsGetCount(ctx: HbsContext, key: string): usize;
export fn hbsWriteEntry(ctx: HbsContext, idx: usize, escape: bool, out: []u8, pos: usize): usize;
export fn hbsStripQuotes(s: string): string;
export fn hbsFindSpace(s: string, start: usize): usize;
export fn hbsEvalInline(expr_raw: string, ctx: HbsContext, escape: bool, out: []u8, pos: usize): usize;
export fn hbsFindBlockBounds(tpl: string, body_start: usize, end: usize, block_tag: string): HbsBlockSpan;
export fn hbsRenderRange(tpl: string, start: usize, end: usize, ctx: HbsContext, out: []u8, pos: usize, depth: usize): usize;
export fn renderHandlebars(tpl: string, ctx: HbsContext, out: []u8): usize;
