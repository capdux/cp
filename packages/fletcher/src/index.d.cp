// Type definitions for CP

export class Fletcher16 {
    sum1: u32,
    sum2: u32,
    fn new(): Fletcher16;
    fn init(): Fletcher16;
    fn withInitialValues(a: u8, b: u8): Fletcher16;
    fn update(_: auto, data: string);
    fn updateBytes(_: auto, data: &[]u8);
    fn digest(): u16;
    fn value(): u16;
    fn reset(_: auto);
}

export class Fletcher32 {
    sum1: u32,
    sum2: u32,
    pending: u8,
    has_pending: bool,
    fn new(): Fletcher32;
    fn init(): Fletcher32;
    fn withInitialValues(a: u16, b: u16): Fletcher32;
    fn updateWords(_: auto, words: &[]u16);
    fn updateBytes(_: auto, data: &[]u8);
    fn update(_: auto, data: string);
    fn digest(): u32;
    fn value(): u32;
    fn reset(_: auto);
}

export class Fletcher64 {
    sum1: u64,
    sum2: u64,
    pending: [4]u8,
    pending_len: usize,
    fn new(): Fletcher64;
    fn init(): Fletcher64;
    fn withInitialValues(a: u32, b: u32): Fletcher64;
    fn updateWords(_: auto, words: &[]u32);
    fn updateBytes(_: auto, data: &[]u8);
    fn update(_: auto, data: string);
    fn digest(): u64;
    fn value(): u64;
    fn reset(_: auto);
}

export fn fletcher16(data: string): u16;
export fn fletcher16Bytes(data: &[]u8): u16;
export fn calcFletcher16(data: &[]u8): u16;
export fn fletcher32(data: string): u32;
export fn fletcher32Bytes(data: &[]u8): u32;
export fn calcFletcher32(words: &[]u16): u32;
export fn fletcher64(data: string): u64;
export fn fletcher64Bytes(data: &[]u8): u64;
export fn calcFletcher64(words: &[]u32): u64;
