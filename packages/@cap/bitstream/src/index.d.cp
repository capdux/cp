// Type definitions for CP

export class BitReadResult {
    value: u64,
    ok: bool,
}

export class BitReader {
    buf: &[]u8,
    bit_pos: usize,
    msb_first: bool,
    fn initMsb(buf: &[]u8): BitReader;
    fn initLsb(buf: &[]u8): BitReader;
    fn bitPosition(): usize;
    fn bitsRemaining(): usize;
    fn readBitAt(pos: usize): u8;
    fn readBit(_: auto): BitReadResult;
    fn peekBits(count: u8): BitReadResult;
    fn readBits(_: auto, count: u8): BitReadResult;
    fn skipBits(_: auto, count: usize): bool;
    fn alignToByte(_: auto);
    fn readUe(_: auto): BitReadResult;
}

export class BitWriter {
    buf: []u8,
    bit_pos: usize,
    msb_first: bool,
    fn initMsb(buf: []u8): BitWriter;
    fn initLsb(buf: []u8): BitWriter;
    fn bitsWritten(): usize;
    fn bytesUsed(): usize;
    fn writeBit(_: auto, bit: u8): bool;
    fn writeBits(_: auto, value: u64, count: u8): bool;
    fn alignToByte(_: auto): bool;
    fn writeUe(_: auto, value: u64): bool;
}

