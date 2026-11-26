// Type definitions for CP

export class HuffmanCodec {
    code_len: [MAX_SYMBOLS]u8,
    codes: [MAX_SYMBOLS]u16,
    symbol_count: usize,
    fn empty(): HuffmanCodec;
    fn fromCodeLengths(lengths: &[]u8): HuffmanCodec;
    fn fromFrequencies(freqs: &[]u32): HuffmanCodec;
    fn codeLength(sym: u8): u8;
    fn codeBits(sym: u8): u16;
    fn encode(src: &[]u8, dst: []u8): i64;
    fn decode(src: &[]u8, dst: []u8): i64;
}

