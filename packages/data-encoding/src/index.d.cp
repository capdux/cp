// Type definitions for CP

export class DecodeResult {
    ok: bool,
    len: usize,
    error_pos: usize,
    fn success(n: usize): DecodeResult;
    fn fail(pos: usize): DecodeResult;
}

export fn dencHexDigit(v: u8, upper: bool): u8;
export fn dencHexVal(c: u8): u8;
export fn dencB32Char(v: u8): u8;
export fn dencB32Val(c: u8): u8;
export fn dencB64Char(v: u8, url_safe: bool): u8;
export fn dencB64Val(c: u8, url_safe: bool): u8;
export fn dencHexEncodeImpl(src: &[]u8, upper: bool, out: []u8): usize;
export fn base64EncodeLen(n: usize, pad: bool): usize;
export fn base32EncodeLen(n: usize, pad: bool): usize;
export fn hexLowerEncode(src: &[]u8, out: []u8): usize;
export fn hexUpperEncode(src: &[]u8, out: []u8): usize;
export fn hexDecode(src: &[]u8, out: []u8): DecodeResult;
export fn base32Encode(src: &[]u8, pad: bool, out: []u8): usize;
export fn base32Decode(src: &[]u8, out: []u8): DecodeResult;
export fn base64Encode(src: &[]u8, url_safe: bool, pad: bool, out: []u8): usize;
export fn base64Decode(src: &[]u8, url_safe: bool, out: []u8): DecodeResult;
