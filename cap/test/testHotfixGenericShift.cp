import { print } from "cap:io";

fn check(cond: bool, msg: string) {
    if (!cond) {
        @panic(msg);
    }
}

pub fn maskRightShift(comptime MaskInt: type, comptime ShiftInt: type, n: usize): MaskInt {
    return (~@as(MaskInt, 0)) >> @as(ShiftInt, @intCast(n));
}

pub fn bitLeftShift(comptime MaskInt: type, comptime ShiftInt: type, n: usize): MaskInt {
    return @as(MaskInt, 1) << @as(ShiftInt, @intCast(n));
}

pub fn shiftAssignLeft(comptime MaskInt: type, comptime ShiftInt: type, val: MaskInt, n: usize): MaskInt {
    let acc: MaskInt = val;
    acc <<= @as(ShiftInt, @intCast(n));
    return acc;
}

pub fn shiftAssignRight(comptime MaskInt: type, comptime ShiftInt: type, val: MaskInt, n: usize): MaskInt {
    let acc: MaskInt = val;
    acc >>= @as(ShiftInt, @intCast(n));
    return acc;
}

fn main() {
    // 1. Generic MaskInt >> ShiftInt (repro pattern)
    let r64: u64 = maskRightShift(u64, u6, 60);
    check(r64 == 0xF, "u64 >> u6 maskRightShift");

    let r32: u32 = maskRightShift(u32, u5, 24);
    check(r32 == 0xFF, "u32 >> u5 maskRightShift");

    let r8: u8 = maskRightShift(u8, u3, 4);
    check(r8 == 0x0F, "u8 >> u3 maskRightShift");

    // 2. Generic MaskInt << ShiftInt
    let l64: u64 = bitLeftShift(u64, u6, 40);
    check(l64 == (1 as u64) << 40, "u64 << u6 bitLeftShift");

    let l32: u32 = bitLeftShift(u32, u5, 16);
    check(l32 == 65536, "u32 << u5 bitLeftShift");

    // 3. Compound shift assign <<= and >>=
    let al64: u64 = shiftAssignLeft(u64, u6, 3, 5);
    check(al64 == 96, "u64 <<= u6 shiftAssignLeft");

    let ar64: u64 = shiftAssignRight(u64, u6, 96, 5);
    check(ar64 == 3, "u64 >>= u6 shiftAssignRight");

    // 4. Direct concrete u64 << u6 and u64 >> u6
    let base: u64 = 1;
    let sh: u6 = 10;
    let shifted_l: u64 = base << sh;
    check(shifted_l == 1024, "direct u64 << u6");

    let shifted_r: u64 = shifted_l >> sh;
    check(shifted_r == 1, "direct u64 >> u6");

    print("testHotfixGenericShift PASS");
}
