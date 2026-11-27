// @cap/fnv-hash — Zero-heap FNV-1 & FNV-1a (32-bit & 64-bit) streaming hash functions
// Clean-room MIT implementation for CP (compatible with Rust fnv & npm @sindresorhus/fnv1a).

pub const FNV32_OFFSET_BASIS: u32 = 2166136261;
pub const FNV32_PRIME: u32 = 16777619;

pub fn fnv64OffsetBasis(): u64 {
    // 0xcbf29ce484222325
    return ((0xcbf29ce4 as u64) << 32) | (0x84222325 as u64);
}

pub fn fnv64Prime(): u64 {
    // 0x00000100000001b3
    return ((0x00000100 as u64) << 32) | (0x000001b3 as u64);
}

/// Wrapping 32-bit multiply via u64 to avoid debug-mode overflow panics.
fn mulWrap32(a: u32, b: u32): u32 {
    let prod: u64 = (a as u64) * (b as u64);
    return (prod & 0xffffffff) as u32;
}

/// Wrapping 64-bit multiply by FNV64_PRIME (0x100000001b3) without 128-bit overflow panics:
/// h * (2^40 + 2^8 + 0xb3) = (h << 40) + (h << 8) + h * 0xb3 (all mod 2^64).
fn mulFnv64Prime(h: u64): u64 {
    let lo: u64 = h & 0xffffffff;
    let hi: u64 = (h >> 32) & 0xffffffff;

    // 0x1b3 = 435, 0x100000001b3 = (0x100 << 32) + 0x1b3
    let p_lo: u64 = lo * 0x1b3;
    let carry: u64 = p_lo >> 32;
    let p_hi: u64 = (hi * 0x1b3 + lo * 0x100 + carry) & 0xffffffff;
    return (p_hi << 32) | (p_lo & 0xffffffff);
}

/// Streaming 32-bit FNV-1a update.
pub fn fnv1a32Update(state: u32, data: []const u8): u32 {
    let h: u32 = state;
    let i: usize = 0;
    while (i < data.len) {
        h = h ^ (data[i] as u32);
        h = mulWrap32(h, FNV32_PRIME);
        i += 1;
    }
    return h;
}

/// One-shot 32-bit FNV-1a hash.
pub fn fnv1a32(data: []const u8): u32 {
    return fnv1a32Update(FNV32_OFFSET_BASIS, data);
}

/// One-shot 32-bit FNV-1 hash (multiply then XOR).
pub fn fnv132(data: []const u8): u32 {
    let h: u32 = FNV32_OFFSET_BASIS;
    let i: usize = 0;
    while (i < data.len) {
        h = mulWrap32(h, FNV32_PRIME);
        h = h ^ (data[i] as u32);
        i += 1;
    }
    return h;
}

/// Streaming 64-bit FNV-1a update.
pub fn fnv1a64Update(state: u64, data: []const u8): u64 {
    let h: u64 = state;
    let i: usize = 0;
    while (i < data.len) {
        h = h ^ (data[i] as u64);
        h = mulFnv64Prime(h);
        i += 1;
    }
    return h;
}

/// One-shot 64-bit FNV-1a hash.
pub fn fnv1a64(data: []const u8): u64 {
    return fnv1a64Update(fnv64OffsetBasis(), data);
}

/// One-shot 64-bit FNV-1 hash (multiply then XOR).
pub fn fnv164(data: []const u8): u64 {
    let h: u64 = fnv64OffsetBasis();
    let i: usize = 0;
    while (i < data.len) {
        h = mulFnv64Prime(h);
        h = h ^ (data[i] as u64);
        i += 1;
    }
    return h;
}

/// Format a 64-bit hash as fixed 16-character lowercase hex into `out`.
pub fn formatHex64(h: u64, out: []u8): usize {
    if (out.len < 16) { return 0; }
    let hex: string = "0123456789abcdef";
    let i: usize = 0;
    while (i < 16) {
        let shift: u6 = ((15 - i) * 4) as u6;
        let nib: usize = ((h >> shift) & 0x0f) as usize;
        out[i] = hex[nib];
        i += 1;
    }
    return 16;
}
