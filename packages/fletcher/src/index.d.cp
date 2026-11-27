// Type definitions for CP

export class Fletcher16 {
    sum1: u32,
    sum2: u32,
    fn new(): Fletcher16;
    fn update(_: auto, data: string);
    fn updateBytes(_: auto, data: []u8);
    fn digest(): u16;
}

export fn fletcher16(data: string): u16;
export fn fletcher16Bytes(data: []u8): u16;
export fn fletcher32(data: string): u32;
export fn fletcher32Bytes(data: []u8): u32;
export fn fletcher64(data: string): u64;
