// Type definitions for CP

export fn isFormSafe(b: u8): bool;
export fn upperHex(v: u8): u8;
export fn hexVal(c: u8): i32;
export fn encodedSize(input: string, space_as_plus: bool): usize;
export fn percentEncode(input: string, space_as_plus: bool, out: []u8): usize;
export fn decodedSize(input: string): usize;
export fn percentDecode(input: string, plus_as_space: bool, out: []u8): usize;
export class FormPair {
    key_start: usize,
    key_len: usize,
    val_start: usize,
    val_len: usize,
}

export fn parseFormPairs(query: string, out: []FormPair): usize;
export class FormSerializer {
    len: usize,
    count: usize,
    fn new(): FormSerializer;
    fn appendPair(_: auto, out: []u8, key: string, val: string): bool;
}

export fn decodedMatches(raw: string, plus_as_space: bool, target: string): bool;
export fn getParamDecoded(query: string, target_key: string, out: []u8): i64;
