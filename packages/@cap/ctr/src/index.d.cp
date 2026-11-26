// Type definitions for CP

import { Aes128, Aes256 } from "../../aes/src/index.cp";
export fn ctrZeroBlock(): [16]u8;
export fn ctrAddBe128(iv: [16]u8, delta: u64): [16]u8;
export fn ctrAddLe128(iv: [16]u8, delta: u64): [16]u8;
export class Aes128Ctr128Be {
    cipher: Aes128,
    iv: [16]u8,
    block_idx: u64,
    buf: [16]u8,
    buf_pos: usize,
    fn new(key: [16]u8, iv: [16]u8): Aes128Ctr128Be;
    fn currentPos(): u64;
    fn seek(_: auto, byte_pos: u64);
    fn applyKeystream(_: auto, data: []u8);
}

export class Aes128Ctr128Le {
    cipher: Aes128,
    iv: [16]u8,
    block_idx: u64,
    buf: [16]u8,
    buf_pos: usize,
    fn new(key: [16]u8, iv: [16]u8): Aes128Ctr128Le;
    fn currentPos(): u64;
    fn seek(_: auto, byte_pos: u64);
    fn applyKeystream(_: auto, data: []u8);
}

export class Aes256Ctr128Be {
    cipher: Aes256,
    iv: [16]u8,
    block_idx: u64,
    buf: [16]u8,
    buf_pos: usize,
    fn new(key: [32]u8, iv: [16]u8): Aes256Ctr128Be;
    fn currentPos(): u64;
    fn seek(_: auto, byte_pos: u64);
    fn applyKeystream(_: auto, data: []u8);
}

export class Aes256Ctr128Le {
    cipher: Aes256,
    iv: [16]u8,
    block_idx: u64,
    buf: [16]u8,
    buf_pos: usize,
    fn new(key: [32]u8, iv: [16]u8): Aes256Ctr128Le;
    fn currentPos(): u64;
    fn seek(_: auto, byte_pos: u64);
    fn applyKeystream(_: auto, data: []u8);
}

