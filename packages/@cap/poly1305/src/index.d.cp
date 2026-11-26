// Type definitions for CP

import { ctEqBytes } from "../../subtle-ct/src/index.cp";
export fn p1305ReadU32Le(b: []u8, off: usize): u32;
export fn p1305WriteU32Le(out: []u8, off: usize, v: u32);
export class Poly1305 {
    r0: u32,
    r1: u32,
    r2: u32,
    r3: u32,
    r4: u32,
    pad0: u32,
    pad1: u32,
    pad2: u32,
    pad3: u32,
    h0: u32,
    h1: u32,
    h2: u32,
    h3: u32,
    h4: u32,
    buf: [16]u8,
    buf_len: usize,
    fn init(key32: []u8): Poly1305;
    fn processBlock(_: auto, blk: []u8, hibit: u32);
    fn update(_: auto, data: []u8);
    fn finalize(_: auto, out_tag16: []u8);
}

export fn poly1305Mac(key32: []u8, msg: []u8, out_tag16: []u8): bool;
export fn poly1305Verify(key32: []u8, msg: []u8, expected_tag16: []u8): bool;
