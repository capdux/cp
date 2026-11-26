// Type definitions for CP

export fn aesXtime(x: u8): u8;
export fn aesMul(a_in: u8, b_in: u8): u8;
export fn aesSubBytes(state: []u8);
export fn aesInvSubBytes(state: []u8);
export fn aesShiftRows(s: []u8);
export fn aesInvShiftRows(s: []u8);
export fn aesMixColumns(s: []u8);
export fn aesInvMixColumns(s: []u8);
export fn aesAddRoundKey(s: []u8, rk: &[]u8, round: usize);
export fn aesExpandKey(key: &[]u8, nk: usize, nr: usize, rk: []u8);
export fn aesEncryptCore(block: [16]u8, rk: &[]u8, nr: usize): [16]u8;
export fn aesDecryptCore(block: [16]u8, rk: &[]u8, nr: usize): [16]u8;
export class Aes128 {
    rk: [176]u8,
    fn new(key: [16]u8): Aes128;
    fn encryptBlock(block: [16]u8): [16]u8;
    fn decryptBlock(block: [16]u8): [16]u8;
}

export class Aes192 {
    rk: [208]u8,
    fn new(key: [24]u8): Aes192;
    fn encryptBlock(block: [16]u8): [16]u8;
    fn decryptBlock(block: [16]u8): [16]u8;
}

export class Aes256 {
    rk: [240]u8,
    fn new(key: [32]u8): Aes256;
    fn encryptBlock(block: [16]u8): [16]u8;
    fn decryptBlock(block: [16]u8): [16]u8;
}

