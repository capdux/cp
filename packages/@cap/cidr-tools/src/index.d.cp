// Type definitions for CP

export class Cidr4 {
    network: u32,
    prefix: u8,
    valid: bool,
}

export fn invalidCidr(): Cidr4;
export fn pow2_64(n: u8): u64;
export fn prefixMask(prefix: u8): u32;
export fn parseIpv4(s: &[]u8): i64;
export fn parseCidr4(s: &[]u8): Cidr4;
export fn appendU8Dec(dst: []u8, pos: usize, v_in: u8): usize;
export fn formatCidr4(c: Cidr4, dst: []u8): usize;
export fn cidrHostCount(c: Cidr4): u64;
export fn cidrStart(c: Cidr4): u32;
export fn cidrEnd(c: Cidr4): u32;
export fn containsIp(c: Cidr4, ip: u32): bool;
export fn containsCidr(outer: Cidr4, inner: Cidr4): bool;
export fn overlaps(a: Cidr4, b: Cidr4): bool;
export class Range64 {
    lo: u64,
    hi: u64,
}

export fn emitRangeCidrs(lo_in: u64, hi: u64, out: []Cidr4, pos_in: usize): usize;
export fn mergeCidrs(inputs: &[]Cidr4, out: []Cidr4): usize;
export fn excludeCidr(base: Cidr4, remove: Cidr4, out: []Cidr4): usize;
