// Type definitions for CP

import { writeStr } from "cap:fmt";
import { ObjectIdentifier, OidParseResult } from "../../const-oid/src/index.cp";
export fn derEmptySlice(): &[]u8;
export fn derMinBytesUsize(v: usize): usize;
export class DerHeader {
    tag: u8,
    len: usize,
    header_len: usize,
    ok: bool,
}

export class DerTlvResult {
    tag: u8,
    value: &[]u8,
    ok: bool,
}

export class DerSliceResult {
    data: &[]u8,
    ok: bool,
}

export class DerBoolResult {
    value: bool,
    ok: bool,
}

export class DerU64Result {
    value: u64,
    ok: bool,
}

export class DerBitStringResult {
    unused_bits: u8,
    bytes: &[]u8,
    ok: bool,
}

export class DerSequenceResult {
    reader: DerReader,
    ok: bool,
}

export class DerReader {
    buf: &[]u8,
    pos: usize,
    fn new(buf: &[]u8): DerReader;
    fn remaining(): usize;
    fn isEmpty(): bool;
    fn peekTag(): u8;
    fn readHeader(_: auto): DerHeader;
    fn readTlv(_: auto): DerTlvResult;
    fn readBool(_: auto): DerBoolResult;
    fn readU64(_: auto): DerU64Result;
    fn readOctetString(_: auto): DerSliceResult;
    fn readBitString(_: auto): DerBitStringResult;
    fn readNull(_: auto): bool;
    fn readOid(_: auto): OidParseResult;
    fn readSequence(_: auto): DerSequenceResult;
}

export class DerWriter {
    buf: []u8,
    pos: usize,
    fn new(buf: []u8): DerWriter;
    fn len(): usize;
    fn remaining(): usize;
    fn written(): &[]u8;
    fn encodedLengthSize(length: usize): usize;
    fn encodeLength(length: usize, out: []u8): usize;
    fn writeTlv(_: auto, tag: u8, value: &[]u8): bool;
    fn writeBool(_: auto, val: bool): bool;
    fn writeU64(_: auto, val: u64): bool;
    fn writeOctetString(_: auto, data: &[]u8): bool;
    fn writeBitString(_: auto, unused_bits: u8, data: &[]u8): bool;
    fn writeNull(_: auto): bool;
    fn writeOid(_: auto, oid: ObjectIdentifier): bool;
    fn writeSequence(_: auto, content: &[]u8): bool;
}

