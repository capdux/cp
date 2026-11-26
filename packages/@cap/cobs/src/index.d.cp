// Type definitions for CP

export fn max_encoded_len(source_len: usize): usize;
export fn encode(src: &[]u8, dst: []u8): usize;
export fn decode(src: &[]u8, dst: []u8): i64;
export fn encode_with_sentinel(src: &[]u8, dst: []u8, sentinel: u8): usize;
export fn decode_with_sentinel(src: &[]u8, dst: []u8, sentinel: u8): i64;
export fn encode_framed(src: &[]u8, dst: []u8): usize;
export fn decode_framed(src: &[]u8, dst: []u8): i64;
export class CobsDecoder {
    buf: [512]u8,
    write_len: usize,
    block_rem: u8,
    cur_code: u8,
    has_data: bool,
    failed: bool,
    frame_ready: bool,
    fn init(): CobsDecoder;
    fn reset(_: auto);
    fn frame(): &[]u8;
    fn push(_: auto, byte: u8): i64;
}

