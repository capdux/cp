// Type definitions for CP

export enum HasArg {
    No,
    Yes,
    Maybe
}

export enum Occur {
    Optional,
    Req,
    Multi
}

export class OptDef {
    short_name: string,
    long_name: string,
    desc: string,
    has_arg: HasArg,
    occur: Occur,
    fn empty(): OptDef;
}

export class OptVal {
    opt_idx: usize,
    has_val: bool,
    val: string,
    fn empty(): OptVal;
}

export fn goptStrEq(a: string, b: string): bool;
export fn goptFindEq(s: string): usize?;
export fn goptFindByName(defs: &[]OptDef, count: usize, name: string): usize?;
export fn goptFindShort(defs: &[]OptDef, count: usize, ch: string): usize?;
export fn goptFindLong(defs: &[]OptDef, count: usize, name: string): usize?;
export fn goptCountForIdx(vals: &[]OptVal, val_count: usize, idx: usize): usize;
export class Matches {
    ok: bool,
    err_kind: i32,
    err_opt: string,
    defs: [32]OptDef,
    def_count: usize,
    vals: [64]OptVal,
    val_count: usize,
    free: [64]string,
    free_count: usize,
    fn optPresent(name: string): bool;
    fn optCount(name: string): usize;
    fn optStr(name: string): string?;
    fn optStrs(name: string, out: []string): usize;
    fn freeCount(): usize;
    fn freeAt(idx: usize): string?;
}

export class Options {
    defs: [32]OptDef,
    def_count: usize,
    fn new(): Options;
    fn opt(_: auto, short: string, long: string, desc: string, has_arg: HasArg, occur: Occur);
    fn optflag(_: auto, short: string, long: string, desc: string);
    fn optflagmulti(_: auto, short: string, long: string, desc: string);
    fn optopt(_: auto, short: string, long: string, desc: string);
    fn optmulti(_: auto, short: string, long: string, desc: string);
    fn reqopt(_: auto, short: string, long: string, desc: string);
    fn parse(args: &[]string): Matches;
}

