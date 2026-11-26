// Type definitions for CP

export fn encodePackBits(input: string, out: []u8): string?;
export fn decodePackBits(encoded: string, out: []u8): string?;
export fn encodeDeltaI64(values: &[]i64, out: []i64): usize;
export fn decodeDeltaI64(deltas: &[]i64, out: []i64): usize;
export fn encodeDeltaOfDeltaI64(values: &[]i64, out: []i64): usize;
export fn decodeDeltaOfDeltaI64(dod: &[]i64, out: []i64): usize;
