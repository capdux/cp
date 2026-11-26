// Type definitions for CP

import { TAG_INTEGER, DerReader, DerWriter } from "../../der-asn1/src/index.cp";
import { ctEqBytes } from "../../subtle-ct/src/index.cp";
export fn ecsigZero48(): [48]u8;
export fn ecsigIsAllZero(data: &[]u8): bool;
export fn ecsigP256Order(): [32]u8;
export fn ecsigP256HalfOrder(): [32]u8;
export fn ecsigCmpBe32(a: &[]u8, b: &[]u8): i32;
export fn ecsigWriteBeInt(w: &mut DerWriter, val: &[]u8): bool;
export fn ecsigReadBeInt(r: &mut DerReader, coord_len: usize, out: []u8): i64;
export class EcdsaSigResult {
    ok: bool,
    sig: EcdsaSignature,
    err: i64,
}

export class EcdsaSignature {
    r: [48]u8,
    s: [48]u8,
    coord_len: usize,
    fn empty(): EcdsaSignature;
    fn fromFixed(bytes: &[]u8): EcdsaSigResult;
    fn fromDer(der: &[]u8, coord_len: usize): EcdsaSigResult;
    fn toFixed(out: []u8): usize;
    fn toDer(out: []u8): i64;
    fn isLowSP256(): bool;
    fn normalizeLowSP256(_: auto): bool;
    fn ctEq(other: EcdsaSignature): bool;
}

