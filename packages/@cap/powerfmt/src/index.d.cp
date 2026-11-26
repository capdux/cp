// Type definitions for CP

export class IntFmtSpec {
    min_width: usize,
    pad_byte: u8,
    alignment: u8,
    radix: u8,
    uppercase: bool,
    force_sign: bool,
    group_sep: u8,
    group_size: usize,
    fn defaultSpec(): IntFmtSpec;
}

export fn digitByte(d: u64, uppercase: bool): u8;
export fn writeByte(out: []u8, pos: usize, b: u8): usize;
export fn writeRepeat(out: []u8, pos: usize, b: u8, count: usize): usize;
export fn formatMagnitude(mag: u64, sign_byte: u8, spec: IntFmtSpec, out: []u8): usize;
export fn formatU64(val: u64, spec: IntFmtSpec, out: []u8): usize;
export fn formatI64(val: i64, spec: IntFmtSpec, out: []u8): usize;
export fn formatDecPadded(val: i64, width: usize, out: []u8): usize;
export fn formatHex(val: u64, uppercase: bool, out: []u8): usize;
export fn formatWithCommas(val: i64, out: []u8): usize;
