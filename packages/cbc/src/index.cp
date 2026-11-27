// @cap/cbc — Zero-allocation pure CP AES-128-CBC & AES-256-CBC with PKCS#7 padding
// Parity with RustCrypto crates.io:cbc (MIT)
// Accelerated with 16-lane @Vector(16, u8) SIMD block XOR and bulk @memcpy / @memset.

import { Aes128, Aes256 } from "../../aes/src/index.cp";

type CbcVec16 = @Vector(16, u8);

fn cbcmReadBlock(src: []const u8, offset: usize): [16]u8 {
    let blk: [16]u8 = undefined;
    @memcpy(blk[0..16], src[offset .. offset + 16]);
    return blk;
}

fn cbcmWriteBlock(dst: []u8, offset: usize, blk: [16]u8) {
    @memcpy(dst[offset .. offset + 16], blk[0..16]);
}

fn cbcmXorBlock(a: [16]u8, b: [16]u8): [16]u8 {
    const va: CbcVec16 = a;
    const vb: CbcVec16 = b;
    const out: [16]u8 = va ^ vb;
    return out;
}

fn cbcmValidatePkcs7(lastBlock: [16]u8): i64 {
    const padByte: u8 = lastBlock[15];
    const padLen: usize = padByte as usize;
    if (padLen < 1 || padLen > 16) {
        return -1;
    }
    const start: usize = 16 - padLen;
    let bad: u8 = 0;
    let i: usize = start;
    while (i < 16) {
        bad = bad | (lastBlock[i] ^ padByte);
        i += 1;
    }
    if (bad != (0 as u8)) {
        return -1;
    }
    return padLen as i64;
}

/// AES-128-CBC whole-block encryption without padding (NIST SP 800-38A).
/// Returns bytes written (plaintext.len), or 0 if plaintext is empty, not a multiple of 16, or out is too small.
pub fn aes128CbcEncryptBlocks(key: [16]u8, iv: [16]u8, plaintext: []const u8, out: []u8): usize {
    if (plaintext.len == 0 || (plaintext.len % 16) != 0 || out.len < plaintext.len) {
        return 0;
    }
    const cipher = Aes128.new(key);
    let prev: [16]u8 = iv;
    let offset: usize = 0;
    while (offset < plaintext.len) {
        const ptBlk = cbcmReadBlock(plaintext, offset);
        const xored = cbcmXorBlock(ptBlk, prev);
        const ctBlk = cipher.encryptBlock(xored);
        cbcmWriteBlock(out, offset, ctBlk);
        prev = ctBlk;
        offset += 16;
    }
    return plaintext.len;
}

/// AES-128-CBC whole-block decryption without padding (NIST SP 800-38A).
/// Returns bytes written (ciphertext.len), or 0 if ciphertext is empty, not a multiple of 16, or out is too small.
pub fn aes128CbcDecryptBlocks(key: [16]u8, iv: [16]u8, ciphertext: []const u8, out: []u8): usize {
    if (ciphertext.len == 0 || (ciphertext.len % 16) != 0 || out.len < ciphertext.len) {
        return 0;
    }
    const cipher = Aes128.new(key);
    let prev: [16]u8 = iv;
    let offset: usize = 0;
    while (offset < ciphertext.len) {
        const ctBlk = cbcmReadBlock(ciphertext, offset);
        const decBlk = cipher.decryptBlock(ctBlk);
        const ptBlk = cbcmXorBlock(decBlk, prev);
        cbcmWriteBlock(out, offset, ptBlk);
        prev = ctBlk;
        offset += 16;
    }
    return ciphertext.len;
}

/// AES-128-CBC encryption with PKCS#7 padding.
/// Returns total ciphertext bytes written, or 0 if out is too small.
pub fn aes128CbcEncryptPkcs7(key: [16]u8, iv: [16]u8, plaintext: []const u8, out: []u8): usize {
    const rem: usize = plaintext.len % 16;
    const padLen: usize = 16 - rem;
    const totalLen: usize = plaintext.len + padLen;
    if (out.len < totalLen) {
        return 0;
    }
    const cipher = Aes128.new(key);
    let prev: [16]u8 = iv;
    const fullBytes: usize = plaintext.len - rem;
    let offset: usize = 0;
    while (offset < fullBytes) {
        const ptBlk = cbcmReadBlock(plaintext, offset);
        const xored = cbcmXorBlock(ptBlk, prev);
        const ctBlk = cipher.encryptBlock(xored);
        cbcmWriteBlock(out, offset, ctBlk);
        prev = ctBlk;
        offset += 16;
    }
    let lastBlk: [16]u8 = undefined;
    const padByte: u8 = padLen as u8;
    if (rem > 0) {
        @memcpy(lastBlk[0..rem], plaintext[fullBytes .. fullBytes + rem]);
    }
    @memset(lastBlk[rem..16], padByte);
    const xoredLast = cbcmXorBlock(lastBlk, prev);
    const ctLast = cipher.encryptBlock(xoredLast);
    cbcmWriteBlock(out, fullBytes, ctLast);
    return totalLen;
}

/// AES-128-CBC decryption with strict PKCS#7 padding verification.
/// Returns plaintext length (>= 0), or -1 on invalid length, invalid padding, or insufficient output buffer.
pub fn aes128CbcDecryptPkcs7(key: [16]u8, iv: [16]u8, ciphertext: []const u8, out: []u8): i64 {
    if (ciphertext.len < 16 || (ciphertext.len % 16) != 0) {
        return -1;
    }
    const cipher = Aes128.new(key);
    const lastOffset: usize = ciphertext.len - 16;
    let lastPrev: [16]u8 = iv;
    if (lastOffset >= 16) {
        lastPrev = cbcmReadBlock(ciphertext, lastOffset - 16);
    }
    const lastCt = cbcmReadBlock(ciphertext, lastOffset);
    const lastDec = cipher.decryptBlock(lastCt);
    const lastPt = cbcmXorBlock(lastDec, lastPrev);
    const padLenI64: i64 = cbcmValidatePkcs7(lastPt);
    if (padLenI64 < 0) {
        return -1;
    }
    const padLen: usize = padLenI64 as usize;
    const plainLen: usize = ciphertext.len - padLen;
    if (out.len < plainLen) {
        return -1;
    }
    let prev: [16]u8 = iv;
    let offset: usize = 0;
    while (offset < lastOffset) {
        const ctBlk = cbcmReadBlock(ciphertext, offset);
        const decBlk = cipher.decryptBlock(ctBlk);
        const ptBlk = cbcmXorBlock(decBlk, prev);
        cbcmWriteBlock(out, offset, ptBlk);
        prev = ctBlk;
        offset += 16;
    }
    const rem: usize = 16 - padLen;
    if (rem > 0) {
        @memcpy(out[lastOffset .. lastOffset + rem], lastPt[0..rem]);
    }
    return plainLen as i64;
}

/// AES-256-CBC whole-block encryption without padding (NIST SP 800-38A).
/// Returns bytes written (plaintext.len), or 0 if plaintext is empty, not a multiple of 16, or out is too small.
pub fn aes256CbcEncryptBlocks(key: [32]u8, iv: [16]u8, plaintext: []const u8, out: []u8): usize {
    if (plaintext.len == 0 || (plaintext.len % 16) != 0 || out.len < plaintext.len) {
        return 0;
    }
    const cipher = Aes256.new(key);
    let prev: [16]u8 = iv;
    let offset: usize = 0;
    while (offset < plaintext.len) {
        const ptBlk = cbcmReadBlock(plaintext, offset);
        const xored = cbcmXorBlock(ptBlk, prev);
        const ctBlk = cipher.encryptBlock(xored);
        cbcmWriteBlock(out, offset, ctBlk);
        prev = ctBlk;
        offset += 16;
    }
    return plaintext.len;
}

/// AES-256-CBC whole-block decryption without padding (NIST SP 800-38A).
/// Returns bytes written (ciphertext.len), or 0 if ciphertext is empty, not a multiple of 16, or out is too small.
pub fn aes256CbcDecryptBlocks(key: [32]u8, iv: [16]u8, ciphertext: []const u8, out: []u8): usize {
    if (ciphertext.len == 0 || (ciphertext.len % 16) != 0 || out.len < ciphertext.len) {
        return 0;
    }
    const cipher = Aes256.new(key);
    let prev: [16]u8 = iv;
    let offset: usize = 0;
    while (offset < ciphertext.len) {
        const ctBlk = cbcmReadBlock(ciphertext, offset);
        const decBlk = cipher.decryptBlock(ctBlk);
        const ptBlk = cbcmXorBlock(decBlk, prev);
        cbcmWriteBlock(out, offset, ptBlk);
        prev = ctBlk;
        offset += 16;
    }
    return ciphertext.len;
}

/// AES-256-CBC encryption with PKCS#7 padding.
/// Returns total ciphertext bytes written, or 0 if out is too small.
pub fn aes256CbcEncryptPkcs7(key: [32]u8, iv: [16]u8, plaintext: []const u8, out: []u8): usize {
    const rem: usize = plaintext.len % 16;
    const padLen: usize = 16 - rem;
    const totalLen: usize = plaintext.len + padLen;
    if (out.len < totalLen) {
        return 0;
    }
    const cipher = Aes256.new(key);
    let prev: [16]u8 = iv;
    const fullBytes: usize = plaintext.len - rem;
    let offset: usize = 0;
    while (offset < fullBytes) {
        const ptBlk = cbcmReadBlock(plaintext, offset);
        const xored = cbcmXorBlock(ptBlk, prev);
        const ctBlk = cipher.encryptBlock(xored);
        cbcmWriteBlock(out, offset, ctBlk);
        prev = ctBlk;
        offset += 16;
    }
    let lastBlk: [16]u8 = undefined;
    const padByte: u8 = padLen as u8;
    if (rem > 0) {
        @memcpy(lastBlk[0..rem], plaintext[fullBytes .. fullBytes + rem]);
    }
    @memset(lastBlk[rem..16], padByte);
    const xoredLast = cbcmXorBlock(lastBlk, prev);
    const ctLast = cipher.encryptBlock(xoredLast);
    cbcmWriteBlock(out, fullBytes, ctLast);
    return totalLen;
}

/// AES-256-CBC decryption with strict PKCS#7 padding verification.
/// Returns plaintext length (>= 0), or -1 on invalid length, invalid padding, or insufficient output buffer.
pub fn aes256CbcDecryptPkcs7(key: [32]u8, iv: [16]u8, ciphertext: []const u8, out: []u8): i64 {
    if (ciphertext.len < 16 || (ciphertext.len % 16) != 0) {
        return -1;
    }
    const cipher = Aes256.new(key);
    const lastOffset: usize = ciphertext.len - 16;
    let lastPrev: [16]u8 = iv;
    if (lastOffset >= 16) {
        lastPrev = cbcmReadBlock(ciphertext, lastOffset - 16);
    }
    const lastCt = cbcmReadBlock(ciphertext, lastOffset);
    const lastDec = cipher.decryptBlock(lastCt);
    const lastPt = cbcmXorBlock(lastDec, lastPrev);
    const padLenI64: i64 = cbcmValidatePkcs7(lastPt);
    if (padLenI64 < 0) {
        return -1;
    }
    const padLen: usize = padLenI64 as usize;
    const plainLen: usize = ciphertext.len - padLen;
    if (out.len < plainLen) {
        return -1;
    }
    let prev: [16]u8 = iv;
    let offset: usize = 0;
    while (offset < lastOffset) {
        const ctBlk = cbcmReadBlock(ciphertext, offset);
        const decBlk = cipher.decryptBlock(ctBlk);
        const ptBlk = cbcmXorBlock(decBlk, prev);
        cbcmWriteBlock(out, offset, ptBlk);
        prev = ctBlk;
        offset += 16;
    }
    const rem: usize = 16 - padLen;
    if (rem > 0) {
        @memcpy(out[lastOffset .. lastOffset + rem], lastPt[0..rem]);
    }
    return plainLen as i64;
}
