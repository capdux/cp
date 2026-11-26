// Type definitions for CP

export fn s20Rotl(v: u32, shift: u5): u32;
export fn s20LoadLe32(src: &[]u8, off: usize): u32;
export fn s20StoreLe32(v: u32, dst: []u8, off: usize);
export fn salsa20Block(key: &[]u8, nonce: &[]u8, counter: u64, out: []u8);
export fn salsa20Apply(key: &[]u8, nonce: &[]u8, initial_counter: u64, input: &[]u8, out: []u8): usize;
