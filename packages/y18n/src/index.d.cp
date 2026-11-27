// Type definitions for CP

export class Y18nEntry {
    key: string,
    singular: string,
    plural: string,
    has_plural: bool,
}

export fn y18StrEq(a: string, b: string): bool;
export fn y18WriteByte(out: []u8, pos: usize, b: u8): usize;
export fn y18WriteBytes(out: []u8, pos: usize, s: string): usize;
export fn y18WriteI64(out: []u8, pos: usize, val: i64): usize;
export fn formatPrintf(tmpl: string, count: i64, has_count: bool, args: &[]string, out: []u8): usize;
export class Y18nCatalog {
    locale: string,
    count: usize,
    entries: [64]Y18nEntry,
    fn new(locale: string): Y18nCatalog;
    fn setLocale(_: auto, locale: string);
    fn getLocale(): string;
    fn clear(_: auto);
    fn findIndex(key: string): i64;
    fn hasEntry(key: string): bool;
    fn setEntry(_: auto, key: string, translation: string): bool;
    fn setPluralEntry(_: auto, singular_key: string, singular_tr: string, plural_tr: string): bool;
    fn t(key: string, args: &[]string, out: []u8): usize;
    fn tn(singular: string, plural: string, count: i64, args: &[]string, out: []u8): usize;
}

