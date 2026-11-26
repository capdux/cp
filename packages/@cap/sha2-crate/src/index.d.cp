// Type definitions for CP

import { Sha256State, sha256Init, sha256Update, sha256Final } from "../../cookie-signature/src/index.cp";
export fn sh2HexDigit(n: u8): u8;
export fn sh2EncodeHex(data: []u8, out: []u8): usize;
export class Sha256 {
    st: Sha256State,
    fn new(): Sha256;
    fn update(_: auto, data: []u8);
    fn updateStr(_: auto, s: string);
    fn finalize(_: auto, out32: []u8);
    fn finalizeHex(_: auto, out64: []u8): usize;
    fn reset(_: auto);
}

export class Sha224 {
    st: Sha256State,
    fn new(): Sha224;
    fn update(_: auto, data: []u8);
    fn updateStr(_: auto, s: string);
    fn finalize(_: auto, out28: []u8);
    fn finalizeHex(_: auto, out56: []u8): usize;
}

export fn sha256Hex(s: string, out64: []u8): usize;
export fn sha224Hex(s: string, out56: []u8): usize;
