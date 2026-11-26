// Type definitions for CP

import { escapeHtml } from "../../escape-html/src/index.cp";
export class EjsEntry {
    key: string,
    str_val: string,
    bool_val: bool,
    count_val: usize,
    is_count: bool,
}

export class EjsContext {
    entries: [32]EjsEntry,
    count: usize,
    fn new(): EjsContext;
    fn setStr(_: auto, key: string, val: string): bool;
    fn setBool(_: auto, key: string, val: bool): bool;
    fn setRepeatCount(_: auto, key: string, n: usize): bool;
    fn size(): usize;
}

export fn ejsStrEq(a: string, b: string): bool;
export fn ejsStartsWith(s: string, prefix: string): bool;
export fn ejsIsSpace(b: u8): bool;
export fn ejsTrim(s: string): string;
export fn ejsPushSlice(out: []u8, pos: usize, s: string): usize;
export fn ejsLookupStr(ctx: EjsContext, key: string): string;
export fn ejsLookupBool(ctx: EjsContext, key: string): bool;
export fn ejsLookupCount(ctx: EjsContext, key: string): usize;
export fn ejsFindTagOpen(tpl: string, start_pos: usize): usize;
export fn ejsFindTagClose(tpl: string, start_pos: usize): usize;
export fn ejsFindBlockBounds(tpl: string, body_start: usize, out_pos: []usize): bool;
export fn ejsRenderSlice(tpl: string, ctx: EjsContext, out: []u8, start_out: usize, depth: usize): usize;
export fn renderEjs(tpl: string, ctx: EjsContext, out: []u8): usize;
