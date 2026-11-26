// Type definitions for CP

export fn reflect8(b: u8): u8;
export class Crc8Hasher {
    table: [256]u8,
    state: u8,
    init_val: u8,
    ref_in: bool,
    ref_out: bool,
    xor_out: u8,
    fn initCustom(poly: u8, init_val: u8, ref_in: bool, ref_out: bool, xor_out: u8): Crc8Hasher;
    fn initSmbus(): Crc8Hasher;
    fn initMaxim(): Crc8Hasher;
    fn initSaeJ1850(): Crc8Hasher;
    fn initAutosar(): Crc8Hasher;
    fn initCdma2000(): Crc8Hasher;
    fn initBluetooth(): Crc8Hasher;
    fn update(_: auto, data: &[]u8);
    fn finalize(): u8;
    fn reset(_: auto);
}

export fn crc8Smbus(data: &[]u8): u8;
export fn crc8Maxim(data: &[]u8): u8;
export fn crc8SaeJ1850(data: &[]u8): u8;
export fn crc8Autosar(data: &[]u8): u8;
export fn crc8Cdma2000(data: &[]u8): u8;
export fn crc8Bluetooth(data: &[]u8): u8;
