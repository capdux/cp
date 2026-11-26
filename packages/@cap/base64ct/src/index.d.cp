// Type definitions for CP

export class DecodeResult {
    len: usize,
    ok: bool,
}

export fn b64ctInRange(c: u32, lo: u32, hi: u32): u32;
export fn b64ctEq(c: u32, target: u32): u32;
export fn b64ctEncode6Std(x: u8): u8;
export fn b64ctEncode6Url(x: u8): u8;
export fn b64ctDecode6Std(ch: u8): u32;
export fn b64ctDecode6Url(ch: u8): u32;
export fn encodedLen(n: usize, padded: bool): usize;
export fn b64ctEncodeCore(src: []u8, out: []u8, url_safe: bool, padded: bool): usize;
export fn encodeStd(src: []u8, out: []u8): usize;
export fn encodeStdUnpadded(src: []u8, out: []u8): usize;
export fn encodeUrl(src: []u8, out: []u8): usize;
export fn encodeUrlUnpadded(src: []u8, out: []u8): usize;
export fn b64ctZeroOut(out: []u8, n: usize);
export fn decodeStd(src: string, out: []u8): DecodeResult;
export fn decodeUrlUnpadded(src: string, out: []u8): DecodeResult;
