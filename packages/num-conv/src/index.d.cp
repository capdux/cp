// Type definitions for CP

export class CheckedU8 {
    val: u8,
    ok: bool,
}

export class CheckedU16 {
    val: u16,
    ok: bool,
}

export class CheckedU32 {
    val: u32,
    ok: bool,
}

export class CheckedI8 {
    val: i8,
    ok: bool,
}

export class CheckedI16 {
    val: i16,
    ok: bool,
}

export class CheckedI32 {
    val: i32,
    ok: bool,
}

export fn extendU8ToU64(v: u8): u64;
export fn extendU16ToU64(v: u16): u64;
export fn extendU32ToU64(v: u32): u64;
export fn extendI8ToI64(v: i8): i64;
export fn extendI16ToI64(v: i16): i64;
export fn extendI32ToI64(v: i32): i64;
export fn truncateU64ToU8(v: u64): u8;
export fn truncateU64ToU16(v: u64): u16;
export fn truncateU64ToU32(v: u64): u32;
export fn tryU64ToU8(v: u64): CheckedU8;
export fn tryU64ToU16(v: u64): CheckedU16;
export fn tryU64ToU32(v: u64): CheckedU32;
export fn tryI64ToI8(v: i64): CheckedI8;
export fn tryI64ToI16(v: i64): CheckedI16;
export fn tryI64ToI32(v: i64): CheckedI32;
export fn tryI64ToU8(v: i64): CheckedU8;
export fn tryI64ToU32(v: i64): CheckedU32;
export fn saturatingI64ToU8(v: i64): u8;
export fn saturatingI64ToI8(v: i64): i8;
export fn saturatingI64ToI16(v: i64): i16;
export fn saturatingU64ToU32(v: u64): u32;
