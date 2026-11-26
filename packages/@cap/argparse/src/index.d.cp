// Type definitions for CP

export enum ArgAction {
    Store,
    StoreTrue,
    StoreFalse,
    Count
}

export class ArgDef {
    short_flag: u8,
    long_name: string,
    dest: string,
    action: ArgAction,
    required: bool,
    default_val: string,
    is_positional: bool,
}

export class ArgSlot {
    dest: string,
    action: ArgAction,
    has_str: bool,
    str_val: string,
    bool_val: bool,
    count_val: u32,
    seen: bool,
}

export fn argpStrEq(a: string, b: string): bool;
export fn argpStripDashes(s: string): string;
export fn argpFindEq(s: string): usize?;
export fn argpResolveDest(def: ArgDef): string;
export class ParseResult {
    ok: bool,
    err_code: i32,
    slot_count: usize,
    slots: [32]ArgSlot,
    pos_count: usize,
    positionals: [32]string,
    fn getString(dest: string): string?;
    fn getBool(dest: string): bool;
    fn getCount(dest: string): u32;
    fn positionalCount(): usize;
    fn positionalAt(idx: usize): string?;
}

export fn argpFindLong(defs: &[]ArgDef, name: string): usize?;
export fn argpFindShort(defs: &[]ArgDef, ch: u8): usize?;
export class ArgParser {
    def_count: usize,
    defs: [32]ArgDef,
    fn init(): ArgParser;
    fn addArgument(_: auto, def: ArgDef): bool;
    fn parseArgs(argv: &[]string): ParseResult;
}

