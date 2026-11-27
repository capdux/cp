// @cap/num-conv — Zero-heap lossless, truncating, checked & saturating integer conversions
// Matching crates.io #206 num-conv (753M+ downloads). Clean-room MIT in pure CP.

pub struct CheckedU8 { pub val: u8, pub ok: bool }
pub struct CheckedU16 { pub val: u16, pub ok: bool }
pub struct CheckedU32 { pub val: u32, pub ok: bool }
pub struct CheckedI8 { pub val: i8, pub ok: bool }
pub struct CheckedI16 { pub val: i16, pub ok: bool }
pub struct CheckedI32 { pub val: i32, pub ok: bool }

// --- Lossless widening (extend) ---
pub fn extendU8ToU64(v: u8): u64 { return v as u64; }
pub fn extendU16ToU64(v: u16): u64 { return v as u64; }
pub fn extendU32ToU64(v: u32): u64 { return v as u64; }
pub fn extendI8ToI64(v: i8): i64 { return v as i64; }
pub fn extendI16ToI64(v: i16): i64 { return v as i64; }
pub fn extendI32ToI64(v: i32): i64 { return v as i64; }

// --- Explicit bit-preserving narrowing (truncate) ---
pub fn truncateU64ToU8(v: u64): u8 { return (v & 0xff) as u8; }
pub fn truncateU64ToU16(v: u64): u16 { return (v & 0xffff) as u16; }
pub fn truncateU64ToU32(v: u64): u32 { return (v & 0xffffffff) as u32; }

// --- Checked narrowing (exact range validation) ---
pub fn tryU64ToU8(v: u64): CheckedU8 {
    if (v > 255) { return CheckedU8 { val: 0, ok: false }; }
    return CheckedU8 { val: v as u8, ok: true };
}

pub fn tryU64ToU16(v: u64): CheckedU16 {
    if (v > 65535) { return CheckedU16 { val: 0, ok: false }; }
    return CheckedU16 { val: v as u16, ok: true };
}

pub fn tryU64ToU32(v: u64): CheckedU32 {
    if (v > 4294967295) { return CheckedU32 { val: 0, ok: false }; }
    return CheckedU32 { val: v as u32, ok: true };
}

pub fn tryI64ToI8(v: i64): CheckedI8 {
    if (v < -128 || v > 127) { return CheckedI8 { val: 0, ok: false }; }
    return CheckedI8 { val: v as i8, ok: true };
}

pub fn tryI64ToI16(v: i64): CheckedI16 {
    if (v < -32768 || v > 32767) { return CheckedI16 { val: 0, ok: false }; }
    return CheckedI16 { val: v as i16, ok: true };
}

pub fn tryI64ToI32(v: i64): CheckedI32 {
    if (v < -2147483648 || v > 2147483647) { return CheckedI32 { val: 0, ok: false }; }
    return CheckedI32 { val: v as i32, ok: true };
}

pub fn tryI64ToU8(v: i64): CheckedU8 {
    if (v < 0 || v > 255) { return CheckedU8 { val: 0, ok: false }; }
    return CheckedU8 { val: v as u8, ok: true };
}

pub fn tryI64ToU32(v: i64): CheckedU32 {
    if (v < 0 || v > 4294967295) { return CheckedU32 { val: 0, ok: false }; }
    return CheckedU32 { val: v as u32, ok: true };
}

// --- Saturating narrowing ---
pub fn saturatingI64ToU8(v: i64): u8 {
    if (v <= 0) { return 0; }
    if (v >= 255) { return 255; }
    return v as u8;
}

pub fn saturatingI64ToI8(v: i64): i8 {
    if (v <= -128) { return -128; }
    if (v >= 127) { return 127; }
    return v as i8;
}

pub fn saturatingI64ToI16(v: i64): i16 {
    if (v <= -32768) { return -32768; }
    if (v >= 32767) { return 32767; }
    return v as i16;
}

pub fn saturatingU64ToU32(v: u64): u32 {
    if (v >= 4294967295) { return 4294967295; }
    return v as u32;
}
