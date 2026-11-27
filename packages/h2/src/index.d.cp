// Type definitions for CP

export class StaticEntry {
    name: string,
    value: string,
    valid: bool,
}

export class IntDecodeResult {
    value: u64,
    consumed: usize,
    valid: bool,
}

export fn hpackEqBytes(a: &[]u8, b: &[]u8): bool;
export fn hpackEntry(n: string, v: string): StaticEntry;
export fn staticTableGet(index: usize): StaticEntry;
export fn staticTableFind(name: string, value: string): usize;
export fn staticTableFindName(name: string): usize;
export fn encodePrefixedInt(val: u64, prefix_bits: u8, first_byte_mask: u8, out: []u8): usize;
export fn decodePrefixedInt(data: &[]u8, prefix_bits: u8): IntDecodeResult;
export fn encodeIndexedHeader(index: usize, out: []u8): usize;
