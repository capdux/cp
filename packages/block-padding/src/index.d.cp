// Type definitions for CP

import { Choice, ctEqU8, ctLtU64, zeroizeBytes } from "../../subtle-ct/src/index.cp";
export fn bpCopyBytes(dst: []u8, src: &[]u8);
export fn pkcs7Pad(input: &[]u8, block_size: usize, out: []u8): i64;
export fn pkcs7UnpadCt(padded: &[]u8, block_size: usize): i64;
export fn iso7816Pad(input: &[]u8, block_size: usize, out: []u8): i64;
export fn iso7816Unpad(padded: &[]u8, block_size: usize): i64;
export fn ansiX923Pad(input: &[]u8, block_size: usize, out: []u8): i64;
export fn ansiX923Unpad(padded: &[]u8, block_size: usize): i64;
export fn zeroPad(input: &[]u8, block_size: usize, out: []u8): i64;
export fn zeroUnpad(padded: &[]u8, block_size: usize): i64;
export fn ctrIncBe128(counter: []u8);
export fn ctrIncLe128(counter: []u8);
export fn xorBlock(dst: []u8, a: &[]u8, b: &[]u8): bool;
export fn wipeBlock(buf: []u8);
