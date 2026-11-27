// Type definitions for CP

export enum PlistValKind {
    String,
    Integer,
    Real,
    Bool,
    Data,
    Date,
    Invalid
}

export class PlistValue {
    kind: PlistValKind,
    str_val: string,
    int_val: i64,
    bool_val: bool,
}

export fn plstInvalid(): PlistValue;
export fn plstEq(a: string, b: string): bool;
export fn plstStartsAt(hay: string, pos: usize, needle: string): bool;
export fn plstFindSub(hay: string, start: usize, needle: string): usize?;
export fn plstSkipWs(s: string, start: usize): usize;
export fn plstParseI64(s: string): i64;
export fn plstSkipBalanced(xml: string, pos: usize, open_tag: string, close_tag: string): usize;
export fn plstSkipElement(xml: string, pos: usize): usize;
export fn plstParseValueAt(xml: string, pos: usize): PlistValue;
export fn plistGetDictValue(xml: string, key_name: string): PlistValue;
export fn plstWriteBytes(dst: []u8, pos: usize, src: string): usize;
export fn plstFormatI64(val: i64, buf: []u8): usize;
export fn plistWriteDictEntry(key: string, val: PlistValue, out: []u8): usize;
