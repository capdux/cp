// Type definitions for CP

export class PrettyBytesOpts {
    binary: bool,
    bits: bool,
    signed_val: bool,
    space: bool,
    precision: u8,
    fn defaults(): PrettyBytesOpts;
}

export fn pbUnitStr(idx: usize, binary: bool, bits: bool): string;
export fn pbAppendStr(out: []u8, pos: &mut usize, s: string): bool;
export fn pbAppendByte(out: []u8, pos: &mut usize, ch: u8): bool;
export fn pbAppendU64(out: []u8, pos: &mut usize, v: u64): bool;
export fn prettyBytesWith(num: f64, opts: PrettyBytesOpts, out: []u8): usize;
export fn prettyBytes(num: f64, out: []u8): usize;
