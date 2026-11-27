// Type definitions for CP

import { strEq, trim } from "cap:ascii";
import { writeStr } from "cap:fmt";
import { escapeHtml } from "../../escape-html/src/index.cp";
export fn mstCopyBytes(src: string, dst: []u8): usize;
export fn mstAppendSlice(out: []u8, pos: &mut usize, src: &[]u8);
export fn mstAppendByte(out: []u8, pos: &mut usize, b: u8);
export fn mstFindSubstr2(s: string, start_pos: usize, c0: u8, c1: u8): i64;
export fn mstFindSubstr3(s: string, start_pos: usize, c0: u8, c1: u8, c2: u8): i64;
export class MustacheEntry {
    key: [48]u8,
    key_len: usize,
    val_kind: u8,
    str_val: [128]u8,
    str_len: usize,
    bool_val: bool,
    list_count: usize,
    fn empty(): MustacheEntry;
}

export class MustacheContext {
    entries: [32]MustacheEntry,
    count: usize,
    fn new(): MustacheContext;
    fn entryCount(): usize;
    fn setStr(_: auto, key: string, val: string): bool;
    fn setBool(_: auto, key: string, val: bool): bool;
    fn setListCount(_: auto, key: string, list_count: usize): bool;
}

export fn mstLookup(ctx: MustacheContext, key: string): i32;
export fn mstEmitValue(ctx: MustacheContext, key: string, escaped: bool, out: []u8, pos: &mut usize);
export fn mstFindSectionClose(tpl: string, start_pos: usize, section_key: string, out_body_end: &mut usize, out_after_close: &mut usize): bool;
export fn mstRenderSlice(tpl: string, ctx: MustacheContext, out: []u8, pos: &mut usize, depth: usize);
export fn renderMustache(template: string, ctx: MustacheContext, out: []u8): usize;
