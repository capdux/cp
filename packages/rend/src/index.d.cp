// Type definitions for CP

import { readU16BE, readU16LE, readU32BE, readU32LE, readU64BE, readU64LE } from "cap:bin";
export fn rndPackU16Be(v: u16): [2]u8;
export fn rndPackU16Le(v: u16): [2]u8;
export fn rndUnpackU16Be(b: [2]u8): u16;
export fn rndUnpackU16Le(b: [2]u8): u16;
export fn rndPackU32Be(v: u32): [4]u8;
export fn rndPackU32Le(v: u32): [4]u8;
export fn rndUnpackU32Be(b: [4]u8): u32;
export fn rndUnpackU32Le(b: [4]u8): u32;
export fn rndPackU64Be(v: u64): [8]u8;
export fn rndPackU64Le(v: u64): [8]u8;
export fn rndUnpackU64Be(b: [8]u8): u64;
export fn rndUnpackU64Le(b: [8]u8): u64;
export class U16Be {
    bytes: [2]u8,
    fn fromNative(v: u16): U16Be;
    fn fromBytes(b: [2]u8): U16Be;
    fn toNative(): u16;
    fn asBytes(): [2]u8;
}

export class U16Le {
    bytes: [2]u8,
    fn fromNative(v: u16): U16Le;
    fn fromBytes(b: [2]u8): U16Le;
    fn toNative(): u16;
    fn asBytes(): [2]u8;
}

export class U32Be {
    bytes: [4]u8,
    fn fromNative(v: u32): U32Be;
    fn fromBytes(b: [4]u8): U32Be;
    fn toNative(): u32;
    fn asBytes(): [4]u8;
}

export class U32Le {
    bytes: [4]u8,
    fn fromNative(v: u32): U32Le;
    fn fromBytes(b: [4]u8): U32Le;
    fn toNative(): u32;
    fn asBytes(): [4]u8;
}

export class I32Be {
    bytes: [4]u8,
    fn fromNative(v: i32): I32Be;
    fn fromBytes(b: [4]u8): I32Be;
    fn toNative(): i32;
    fn asBytes(): [4]u8;
}

export class I32Le {
    bytes: [4]u8,
    fn fromNative(v: i32): I32Le;
    fn fromBytes(b: [4]u8): I32Le;
    fn toNative(): i32;
    fn asBytes(): [4]u8;
}

export class U64Be {
    bytes: [8]u8,
    fn fromNative(v: u64): U64Be;
    fn fromBytes(b: [8]u8): U64Be;
    fn toNative(): u64;
    fn asBytes(): [8]u8;
}

export class U64Le {
    bytes: [8]u8,
    fn fromNative(v: u64): U64Le;
    fn fromBytes(b: [8]u8): U64Le;
    fn toNative(): u64;
    fn asBytes(): [8]u8;
}

export class I64Be {
    bytes: [8]u8,
    fn fromNative(v: i64): I64Be;
    fn fromBytes(b: [8]u8): I64Be;
    fn toNative(): i64;
    fn asBytes(): [8]u8;
}

export class I64Le {
    bytes: [8]u8,
    fn fromNative(v: i64): I64Le;
    fn fromBytes(b: [8]u8): I64Le;
    fn toNative(): i64;
    fn asBytes(): [8]u8;
}

export fn readU16Be(buf: &[]u8, offset: usize): u16?;
export fn readU16Le(buf: &[]u8, offset: usize): u16?;
export fn readU32Be(buf: &[]u8, offset: usize): u32?;
export fn readU32Le(buf: &[]u8, offset: usize): u32?;
export fn readU64Be(buf: &[]u8, offset: usize): u64?;
export fn readU64Le(buf: &[]u8, offset: usize): u64?;
