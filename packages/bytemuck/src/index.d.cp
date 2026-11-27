// Type definitions for CP

import {  } from "std";
import { readU16LE, readU16BE, readU32LE, readU32BE, readU64LE, readU64BE, writeU16LE, writeU16BE, writeU32LE, writeU32BE, writeU64LE, writeU64BE } from "cap:bin";
export class CastSliceResult {
    count: usize,
    err_code: u8,
    fn isOk(): bool;
}

export fn checkCastLayout(byte_len: usize, elem_size: usize, addr: usize, align_req: usize): u8;
export fn isAligned(addr: usize, align_req: usize): bool;
export fn fillZeroed(buf: []u8);
export fn zeroedU8(): u8;
export fn zeroedU16(): u16;
export fn zeroedU32(): u32;
export fn zeroedU64(): u64;
export fn zeroedI32(): i32;
export fn zeroedI64(): i64;
export fn zeroedF32(): f32;
export fn zeroedF64(): f64;
export fn writeU16Le(v: u16, out: []u8): bool;
export fn readU16Le(buf: []u8): u16?;
export fn writeU16Be(v: u16, out: []u8): bool;
export fn readU16Be(buf: []u8): u16?;
export fn writeU32Le(v: u32, out: []u8): bool;
export fn readU32Le(buf: []u8): u32?;
export fn writeU32Be(v: u32, out: []u8): bool;
export fn readU32Be(buf: []u8): u32?;
export fn writeU64Le(v: u64, out: []u8): bool;
export fn readU64Le(buf: []u8): u64?;
export fn writeU64Be(v: u64, out: []u8): bool;
export fn readU64Be(buf: []u8): u64?;
export fn writeF32Le(v: f32, out: []u8): bool;
export fn readF32Le(buf: &[]u8): f32?;
export fn writeF64Le(v: f64, out: []u8): bool;
export fn readF64Le(buf: &[]u8): f64?;
export fn bytesOfU16(v: u16): [2]u8;
export fn bytesOfU32(v: u32): [4]u8;
export fn bytesOfU64(v: u64): [8]u8;
export fn bytesOfF32(v: f32): [4]u8;
export fn bytesOfF64(v: f64): [8]u8;
export fn fromBytesU16(buf: &[]u8): u16?;
export fn fromBytesU32(buf: &[]u8): u32?;
export fn fromBytesU64(buf: &[]u8): u64?;
export fn fromBytesF32(buf: &[]u8): f32?;
export fn fromBytesF64(buf: &[]u8): f64?;
export fn podReadUnalignedU16(buf: &[]u8): u16?;
export fn podReadUnalignedU32(buf: &[]u8): u32?;
export fn podReadUnalignedU64(buf: &[]u8): u64?;
export fn podReadUnalignedI32(buf: &[]u8): i32?;
export fn podReadUnalignedF32(buf: &[]u8): f32?;
export fn podReadUnalignedF64(buf: &[]u8): f64?;
export fn castU16SliceToBytesLe(src: &[]u16, dst: []u8): usize?;
export fn castBytesToU16SliceLe(src: &[]u8, dst: []u16): usize?;
export fn castU32SliceToBytesLe(src: []u32, dst: []u8): usize?;
export fn castBytesToU32SliceLe(src: []u8, dst: []u32): usize?;
export fn castU64SliceToBytesLe(src: &[]u64, dst: []u8): usize?;
export fn castBytesToU64SliceLe(src: &[]u8, dst: []u64): usize?;
export fn tryCastBytesToU16Slice(src: &[]u8, dst: []u16): CastSliceResult;
export fn tryCastBytesToU32Slice(src: &[]u8, dst: []u32): CastSliceResult;
export fn tryCastBytesToU64Slice(src: &[]u8, dst: []u64): CastSliceResult;
export fn castU32SliceAsBytes(src: &[]u32): &[]u8;
export fn castU16SliceAsBytes(src: &[]u16): &[]u8;
export fn castU64SliceAsBytes(src: &[]u64): &[]u8;
export fn tryCastSliceBytesToU32(src: &[]u8): (&[]u32)?;
export fn swapBytesU16(v: u16): u16;
export fn swapBytesU32(v: u32): u32;
export fn swapBytesU64(v: u64): u64;
