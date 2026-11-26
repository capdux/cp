// Type definitions for CP

export fn cc20Add32(a: u32, b: u32): u32;
export fn cc20Rotl16(x: u32): u32;
export fn cc20Rotl12(x: u32): u32;
export fn cc20Rotl8(x: u32): u32;
export fn cc20Rotl7(x: u32): u32;
export fn cc20ReadLe32(b: []u8, off: usize): u32;
export fn cc20WriteLe32(out: []u8, off: usize, v: u32);
export fn cc20QuarterRound(st: []u32, ai: usize, bi: usize, ci: usize, di: usize);
export fn chacha20Block(key: []u8, counter: u32, nonce: []u8, out64: []u8): bool;
export class ChaCha20Cipher {
    key: [32]u8,
    nonce: [12]u8,
    counter: u32,
    block_buf: [64]u8,
    block_pos: usize,
    fn new(key: []u8, nonce: []u8): ChaCha20Cipher?;
    fn newWithCounter(key: []u8, nonce: []u8, counter: u32): ChaCha20Cipher?;
    fn seek(_: auto, block_counter: u32);
    fn applyKeystream(_: auto, data: []u8);
}

