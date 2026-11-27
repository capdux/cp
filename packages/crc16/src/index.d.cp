// Type definitions for CP

export fn reflectU16(v: u16): u16;
export fn crc16BuildTable(poly: u16, ref_in: bool): [256]u16;
export class Crc16Hasher {
    table: [256]u16,
    poly: u16,
    init_val: u16,
    xor_out: u16,
    ref_in: bool,
    ref_out: bool,
    state: u16,
    fn initCustom(poly: u16, init_val: u16, ref_in: bool, ref_out: bool, xor_out: u16): Crc16Hasher;
    fn initCcittFalse(): Crc16Hasher;
    fn initXmodem(): Crc16Hasher;
    fn initModbus(): Crc16Hasher;
    fn initKermit(): Crc16Hasher;
    fn initArc(): Crc16Hasher;
    fn initUsb(): Crc16Hasher;
    fn update(_: auto, data: &[]u8);
    fn finalize(): u16;
    fn reset(_: auto);
}

export fn crc16CcittFalse(data: &[]u8): u16;
export fn crc16Xmodem(data: &[]u8): u16;
export fn crc16Modbus(data: &[]u8): u16;
export fn crc16Kermit(data: &[]u8): u16;
export fn crc16Arc(data: &[]u8): u16;
export fn crc16Usb(data: &[]u8): u16;
