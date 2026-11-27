// Type definitions for CP

export class HexDecodeResult {
    len: usize,
    ok: bool,
}

export fn b16InRange(c: u32, lo: u32, hi: u32): u32;
export fn b16EncodeNibbleLower(nib: u8): u8;
export fn b16EncodeNibbleUpper(nib: u8): u8;
export fn b16DecodeNibble(ch: u8, allow_lower: bool, allow_upper: bool): u32;
export fn b16ZeroOut(out: []u8, n: usize);
export fn encodedLen(n: usize): usize;
export fn encodeLower(src: []u8, out: []u8): usize;
export fn encodeUpper(src: []u8, out: []u8): usize;
export fn b16DecodeCore(hex: string, out: []u8, allow_lower: bool, allow_upper: bool): HexDecodeResult;
export fn decodeLower(hex: string, out: []u8): HexDecodeResult;
export fn decodeUpper(hex: string, out: []u8): HexDecodeResult;
export fn decodeMixed(hex: string, out: []u8): HexDecodeResult;
