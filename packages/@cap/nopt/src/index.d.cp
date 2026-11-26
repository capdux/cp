// Type definitions for CP

export class NoptFlag {
    key: string,
    value: string,
    is_bool: bool,
    bool_val: bool,
}

export class NoptShorthand {
    short_key: string,
    long_key: string,
}

export fn nptStrEq(a: string, b: string): bool;
export fn nptHasPrefix(s: string, prefix: string): bool;
export fn nptIndexOfByte(s: string, ch: u8): usize?;
export fn nptStripLeadingDashes(s: string): string;
export fn nptFindShorthand(shorthands: &[]NoptShorthand, short_name: string): string?;
export class NoptResult {
    flags: [64]NoptFlag,
    flag_count: usize,
    remain: [64]string,
    remain_count: usize,
    fn new(): NoptResult;
    fn getString(key: string): string?;
    fn getBool(key: string): bool?;
    fn has(key: string): bool;
    fn remainCount(): usize;
    fn remainAt(idx: usize): string;
    fn setFlag(_: auto, key: string, value: string, is_bool: bool, bool_val: bool);
    fn pushRemain(_: auto, arg: string);
}

export fn nptIsOptionToken(arg: string): bool;
export fn parseNopt(args: &[]string, shorthands: &[]NoptShorthand): NoptResult;
