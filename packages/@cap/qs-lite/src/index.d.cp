// Type definitions for CP

export class QueryPair {
    key: string,
    value: string,
}

export fn hexVal(c: u8): i32;
export fn hexDigit(n: u8): u8;
export fn isUnreserved(c: u8): bool;
export fn encodeComponent(input: string, space_as_plus: bool, out: []u8): usize;
export fn decodeComponent(input: string, plus_as_space: bool, out: []u8): usize;
export fn parseRaw(query: string, out: []QueryPair): usize;
export fn countPairs(query: string): usize;
export fn eqBytes(a: string, b: string): bool;
export fn getDecoded(query: string, target_key: string, out: []u8, out_len: &mut usize): bool;
export fn stringify(pairs: &[]QueryPair, space_as_plus: bool, out: []u8): usize;
