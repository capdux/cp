// Source: cp/cap/test/test_pickle_production_deep.cp (Production deep tests for cap:pickle)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    OP_PROTO,
    OP_FRAME,
    OP_STOP,
    OP_NONE,
    OP_NEWTRUE,
    OP_NEWFALSE,
    OP_BININT,
    OP_BININT1,
    OP_LONG1,
    OP_BINFLOAT,
    OP_SHORT_BINUNICODE,
    OP_BINUNICODE8,
    OP_BINBYTES8,
    OP_EMPTY_DICT,
    OP_MARK,
    OP_SETITEMS,
    OP_MEMOIZE,
    OP_GLOBAL,
    OP_REDUCE,
    OP_TUPLE1,
    detectPickleProto,
    isPickle,
    PickleOp,
    PickleAudit,
    PickleIter,
    auditPickle,
} from "cap:pickle";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn putU32LE(buf: []u8, pos: usize, val: u32): usize {
    buf[pos] = (val & 0xFF) as u8;
    buf[pos + 1] = ((val >> 8) & 0xFF) as u8;
    buf[pos + 2] = ((val >> 16) & 0xFF) as u8;
    buf[pos + 3] = ((val >> 24) & 0xFF) as u8;
    return pos + 4;
}

fn putU64LE(buf: []u8, pos: usize, val: u64): usize {
    let p = putU32LE(buf, pos, (val & 0xFFFF_FFFF) as u32);
    return putU32LE(buf, p, ((val >> 32) & 0xFFFF_FFFF) as u32);
}

fn putU64BE(buf: []u8, pos: usize, val: u64): usize {
    buf[pos] = ((val >> 56) & 0xFF) as u8;
    buf[pos + 1] = ((val >> 48) & 0xFF) as u8;
    buf[pos + 2] = ((val >> 40) & 0xFF) as u8;
    buf[pos + 3] = ((val >> 32) & 0xFF) as u8;
    buf[pos + 4] = ((val >> 24) & 0xFF) as u8;
    buf[pos + 5] = ((val >> 16) & 0xFF) as u8;
    buf[pos + 6] = ((val >> 8) & 0xFF) as u8;
    buf[pos + 7] = (val & 0xFF) as u8;
    return pos + 8;
}

fn putBytes(buf: []u8, pos: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) {
        buf[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

test("1. Protocol 4 pure-data dict with FRAME, SHORT_BINUNICODE, BININT, BINFLOAT, NEWTRUE, SETITEMS, STOP", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let p: usize = 0;

    // PROTO 4
    buf[p] = OP_PROTO; buf[p + 1] = 4; p += 2;
    // FRAME (8B LE length)
    buf[p] = OP_FRAME; p += 1;
    p = putU64LE(buf[0..], p, 34);
    // EMPTY_DICT + MEMOIZE + MARK
    buf[p] = OP_EMPTY_DICT; p += 1;
    buf[p] = OP_MEMOIZE; p += 1;
    buf[p] = OP_MARK; p += 1;
    // key "k" + BININT (-1000)
    buf[p] = OP_SHORT_BINUNICODE; buf[p + 1] = 1; buf[p + 2] = 'k'; p += 3;
    buf[p] = OP_BININT; p += 1;
    let neg_bits: u32 = 0;
    const neg_val: i32 = -1000;
    unsafe { neg_bits = @bitCast(neg_val); }
    p = putU32LE(buf[0..], p, neg_bits);
    // key "f" + BINFLOAT (3.25)
    buf[p] = OP_SHORT_BINUNICODE; buf[p + 1] = 1; buf[p + 2] = 'f'; p += 3;
    buf[p] = OP_BINFLOAT; p += 1;
    let fbits: u64 = 0;
    const fval: f64 = 3.25;
    unsafe { fbits = @bitCast(fval); }
    p = putU64BE(buf[0..], p, fbits);
    // key "b" + NEWTRUE
    buf[p] = OP_SHORT_BINUNICODE; buf[p + 1] = 1; buf[p + 2] = 'b'; p += 3;
    buf[p] = OP_NEWTRUE; p += 1;
    // SETITEMS + STOP
    buf[p] = OP_SETITEMS; p += 1;
    buf[p] = OP_STOP; p += 1;

    expect_eq(isPickle(buf[0..p]), true);
    expect_eq(detectPickleProto(buf[0..p]) ?? 0, 4);

    let it = PickleIter.init(buf[0..p]);
    const op0 = it.next() ?? unreachable;
    expect_eq(op0.opcode, OP_PROTO);
    expect_eq(op0.uint_val, 4);

    const op1 = it.next() ?? unreachable;
    expect_eq(op1.opcode, OP_FRAME);
    expect_eq(op1.uint_val, 34);

    * = it.next() ?? unreachable; // EMPTY_DICT
    * = it.next() ?? unreachable; // MEMOIZE
    * = it.next() ?? unreachable; // MARK

    const op_k = it.next() ?? unreachable;
    expect_eq(sliceEq(op_k.text, "k"), true);
    const op_i = it.next() ?? unreachable;
    expect_eq(op_i.int_val, -1000);

    * = it.next() ?? unreachable; // "f"
    const op_f = it.next() ?? unreachable;
    expect_eq(op_f.float_val > 3.24 && op_f.float_val < 3.26, true);

    * = it.next() ?? unreachable; // "b"
    const op_b = it.next() ?? unreachable;
    expect_eq(op_b.opcode, OP_NEWTRUE);

    * = it.next() ?? unreachable; // SETITEMS
    const op_stop = it.next() ?? unreachable;
    expect_eq(op_stop.opcode, OP_STOP);
    expect_eq(it.next() == null, true);

    const audit = auditPickle(buf[0..p]);
    expect_eq(audit.valid, true);
    expect_eq(audit.proto, 4);
    expect_eq(audit.frame_count, 1);
    expect_eq(audit.global_count, 0);
    expect_eq(audit.reduce_count, 0);
    expect_eq(audit.has_stop, true);
    expect_eq(audit.has_unsafe_ops, false);
});

test("2. Static audit detects executable GLOBAL + REDUCE constructor instructions", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let p: usize = 0;

    buf[p] = OP_PROTO; buf[p + 1] = 2; p += 2;
    // GLOBAL collections\nOrderedDict\n
    buf[p] = OP_GLOBAL; p += 1;
    p = putBytes(buf[0..], p, "collections\nOrderedDict\n");
    buf[p] = OP_SHORT_BINUNICODE; buf[p + 1] = 3; p += 2;
    p = putBytes(buf[0..], p, "arg");
    buf[p] = OP_TUPLE1; p += 1;
    buf[p] = OP_REDUCE; p += 1;
    buf[p] = OP_STOP; p += 1;

    let it = PickleIter.init(buf[0..p]);
    * = it.next() ?? unreachable; // PROTO
    const g = it.next() ?? unreachable;
    expect_eq(g.opcode, OP_GLOBAL);
    expect_eq(sliceEq(g.text, "collections"), true);
    expect_eq(sliceEq(g.text2, "OrderedDict"), true);

    const audit = auditPickle(buf[0..p]);
    expect_eq(audit.valid, true);
    expect_eq(audit.proto, 2);
    expect_eq(audit.global_count, 1);
    expect_eq(audit.reduce_count, 1);
    expect_eq(audit.has_unsafe_ops, true);
    expect_eq(audit.has_stop, true);
});

test("3. Protocol 5 BINUNICODE8, BINBYTES8, and signed LONG1 decoding", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let p: usize = 0;

    buf[p] = OP_PROTO; buf[p + 1] = 5; p += 2;

    // BINUNICODE8 "tensor"
    buf[p] = OP_BINUNICODE8; p += 1;
    p = putU64LE(buf[0..], p, 6);
    p = putBytes(buf[0..], p, "tensor");

    // BINBYTES8 [0xDE, 0xAD, 0xBE, 0xEF]
    buf[p] = OP_BINBYTES8; p += 1;
    p = putU64LE(buf[0..], p, 4);
    buf[p] = 0xDE; buf[p + 1] = 0xAD; buf[p + 2] = 0xBE; buf[p + 3] = 0xEF;
    p += 4;

    // LONG1: 2 bytes LE two's complement for -256 => [0x00, 0xFF]
    buf[p] = OP_LONG1; buf[p + 1] = 2; buf[p + 2] = 0x00; buf[p + 3] = 0xFF;
    p += 4;

    buf[p] = OP_STOP; p += 1;

    let it = PickleIter.init(buf[0..p]);
    const p5 = it.next() ?? unreachable;
    expect_eq(p5.uint_val, 5);

    const u8op = it.next() ?? unreachable;
    expect_eq(u8op.opcode, OP_BINUNICODE8);
    expect_eq(sliceEq(u8op.text, "tensor"), true);

    const b8op = it.next() ?? unreachable;
    expect_eq(b8op.opcode, OP_BINBYTES8);
    expect_eq(b8op.text.len, 4);
    expect_eq(b8op.text[0], 0xDE);
    expect_eq(b8op.text[3], 0xEF);

    const l1op = it.next() ?? unreachable;
    expect_eq(l1op.opcode, OP_LONG1);
    expect_eq(l1op.int_val, -256);

    const audit = auditPickle(buf[0..p]);
    expect_eq(audit.valid, true);
    expect_eq(audit.has_unsafe_ops, false);
});

test("4. detectPickleProto and isPickle reject invalid versions and non-pickle inputs", () => {
    const empty: [1]u8 = [0];
    expect_eq(isPickle(empty[0..0]), false);
    expect_eq(isPickle(empty[0..1]), false);

    // Protocol 1 and Protocol 6 rejected by detectPickleProto (2..5)
    const p1: [2]u8 = [OP_PROTO, 1];
    const p6: [2]u8 = [OP_PROTO, 6];
    expect_eq(isPickle(p1[0..]), false);
    expect_eq(isPickle(p6[0..]), false);
    expect_eq(detectPickleProto(p6[0..]) == null, true);

    const p3: [2]u8 = [OP_PROTO, 3];
    expect_eq(isPickle(p3[0..]), true);
    expect_eq(detectPickleProto(p3[0..]) ?? 0, 3);
});

test("5. Malformed Pickle protection: truncated args, unknown opcode, out-of-bounds length, and missing STOP", () => {
    // Unknown opcode 0xFF
    const bad_op: [3]u8 = [OP_PROTO, 4, 0xFF];
    const a1 = auditPickle(bad_op[0..]);
    expect_eq(a1.valid, false);
    expect_eq(a1.has_stop, false);

    // Truncated BINUNICODE claiming 100 bytes on 6-byte buffer
    const oob_str: [7]u8 = [OP_PROTO, 4, OP_SHORT_BINUNICODE, 100, 'a', 'b', OP_STOP];
    const a2 = auditPickle(oob_str[0..]);
    expect_eq(a2.valid, false);

    // Truncated GLOBAL missing second newline
    const bad_glob: [10]u8 = [OP_PROTO, 2, OP_GLOBAL, 'm', 'o', 'd', '\n', 'f', 'n', OP_STOP];
    const a3 = auditPickle(bad_glob[0..]);
    expect_eq(a3.valid, false);

    // Missing OP_STOP
    const no_stop: [3]u8 = [OP_PROTO, 4, OP_NONE];
    const a4 = auditPickle(no_stop[0..]);
    expect_eq(a4.has_stop, false);
    expect_eq(a4.valid, false);
});
