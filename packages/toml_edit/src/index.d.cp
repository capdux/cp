// Type definitions for CP

export fn tmeStrEq(a: string, b: string): bool;
export fn tmeIsSpace(c: u8): bool;
export fn tmeTrim(s: string): string;
export fn tmeStripComment(s: string): string;
export fn tmeFindChar(s: string, ch: u8): usize?;
export fn tmeFindLastChar(s: string, ch: u8): usize?;
export fn tmeParseInt(raw: string): i64?;
export fn tmeWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn tmeWriteStr(out: []u8, start_pos: usize, s: string): usize;
export fn tmeWriteQuotedStr(out: []u8, start_pos: usize, s: string): usize;
export fn tmeWriteInt(out: []u8, start_pos: usize, val: i64): usize;
export fn tmeWriteEntry(out: []u8, start_pos: usize, e: TomlEntry): usize;
export class TomlEntry {
    section: string,
    key: string,
    kind: u8,
    str_val: string,
    int_val: i64,
    bool_val: bool,
    fn empty(): TomlEntry;
}

export class TomlDoc {
    entries: [64]TomlEntry,
    count: usize,
    fn new(): TomlDoc;
    fn len(): usize;
    fn hasKey(section: string, key: string): bool;
    fn getStr(section: string, key: string): string?;
    fn getInt(section: string, key: string): i64?;
    fn getBool(section: string, key: string): bool?;
    fn setStr(_: auto, section: string, key: string, val: string): bool;
    fn setInt(_: auto, section: string, key: string, val: i64): bool;
    fn setBool(_: auto, section: string, key: string, val: bool): bool;
    fn parse(src: string): TomlDoc;
    fn format(out: []u8): usize;
}

