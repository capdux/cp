// Type definitions for CP

export class Oid {
    valid: bool,
    arcs: [MAX_ARCS]u32,
    arc_count: usize,
}

export fn invalidOid(): Oid;
export fn strEq(a: string, b: string): bool;
export fn isValidOidShape(arcs: &mut [MAX_ARCS]u32, count: usize): bool;
export fn parseOid(dot_str: string): Oid;
export fn appendU32(out: []u8, pos: usize, val_in: u32): usize;
export fn formatOid(oid: Oid, out: []u8): string;
export fn oidEquals(a: Oid, b: Oid): bool;
export fn encodeBase128(val_in: u64, out: []u8, pos: usize): usize?;
export fn encodeOidDer(oid: Oid, out: []u8): string?;
export fn decodeOidDer(der_bytes: string): Oid;
export fn oidName(dot_str: string): string?;
