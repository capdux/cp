// Source: cp/cap/test/test_tftp_production_deep.cp (Production deep tests for cap:tftp)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    TFTP_UDP_PORT,
    TFTP_DEFAULT_BLKSIZE,
    TFTP_DATA_HEADER_SIZE,
    OP_RRQ,
    OP_WRQ,
    OP_DATA,
    OP_ACK,
    OP_ERROR,
    OP_OACK,
    ERR_NOT_DEFINED,
    ERR_FILE_NOT_FOUND,
    ERR_ACCESS_VIOLATION,
    ERR_DISK_FULL,
    ERR_ILLEGAL_OP,
    ERR_UNKNOWN_TID,
    ERR_FILE_EXISTS,
    ERR_NO_SUCH_USER,
    ERR_OPT_NEGOTIATION,
    TftpOption,
    TftpOptIter,
    TftpRequest,
    TftpData,
    TftpAck,
    TftpError,
    TftpOack,
    peekTftpOpcode,
    isTftp,
    parseTftpRequest,
    parseTftpData,
    parseTftpAck,
    parseTftpError,
    parseTftpOack,
    encodeTftpRequest,
    encodeTftpData,
    encodeTftpAck,
    encodeTftpError,
    TftpOptWriter,
} from "cap:tftp";

test("1. RRQ / WRQ with RFC 2347 options (blksize, timeout, tsize) roundtrip and case-insensitive lookup", () => {
    expect_eq(TFTP_UDP_PORT, 69 as u16);
    expect_eq(TFTP_DEFAULT_BLKSIZE, 512);
    expect_eq(TFTP_DATA_HEADER_SIZE, 4);

    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = TftpOptWriter.initRequest(buf[0..], false, "vmlinuz", "octet");
    expect_eq(w.writeOption("BlkSize", "1468"), true);
    expect_eq(w.writeOption("timeout", "5"), true);
    expect_eq(w.writeOption("TSIZE", "0"), true);
    const wire = w.finish() ?? unreachable;
    expect_eq(isTftp(wire), true);
    expect_eq(peekTftpOpcode(wire) ?? 0, OP_RRQ);

    const req = parseTftpRequest(wire) ?? unreachable;
    expect_eq(req.is_write, false);
    expect_eq(std.mem.eql(u8, req.filename, "vmlinuz"), true);
    expect_eq(std.mem.eql(u8, req.mode, "octet"), true);

    // Case-insensitive option lookup
    expect_eq(std.mem.eql(u8, req.findOption("blksize") ?? unreachable, "1468"), true);
    expect_eq(std.mem.eql(u8, req.findOption("TIMEOUT") ?? unreachable, "5"), true);
    expect_eq(std.mem.eql(u8, req.findOption("tsize") ?? unreachable, "0"), true);
    expect_eq(req.findOption("missing") == null, true);

    // Iterate all 3 options
    let it = req.options();
    let count: usize = 0;
    while (true) {
        if (opt := it.next()) {
            * = opt;
            count += 1;
        } else {
            break;
        }
    }
    expect_eq(count, 3);
    expect_eq(it.hasError(), false);

    // Plain WRQ without options
    let wbuf: [64]u8 = std.mem.zeroes([64]u8);
    const wn = encodeTftpRequest(true, "upload.bin", "octet", wbuf[0..]) ?? unreachable;
    const wrq = parseTftpRequest(wbuf[0..wn]) ?? unreachable;
    expect_eq(wrq.is_write, true);
    expect_eq(std.mem.eql(u8, wrq.filename, "upload.bin"), true);
    expect_eq(wrq.options_bytes.len, 0);
});

test("2. DATA and ACK packet encode/decode and isLastBlock detection", () => {
    let dbuf: [600]u8 = std.mem.zeroes([600]u8);
    const full_block: [512]u8 = [0x5A; 512];
    const dn1 = encodeTftpData(1, full_block[0..], dbuf[0..]) ?? unreachable;
    expect_eq(dn1, 516);
    const d1 = parseTftpData(dbuf[0..dn1]) ?? unreachable;
    expect_eq(d1.block, 1 as u16);
    expect_eq(d1.payload.len, 512);
    expect_eq(d1.isLastBlock(TFTP_DEFAULT_BLKSIZE), false);

    // Final short block (12B < 512B)
    const dn2 = encodeTftpData(2, "final-chunk!", dbuf[0..]) ?? unreachable;
    const d2 = parseTftpData(dbuf[0..dn2]) ?? unreachable;
    expect_eq(d2.block, 2 as u16);
    expect_eq(d2.isLastBlock(TFTP_DEFAULT_BLKSIZE), true);
    expect_eq(std.mem.eql(u8, d2.payload, "final-chunk!"), true);

    // ACK packet
    let abuf: [4]u8 = std.mem.zeroes([4]u8);
    const an = encodeTftpAck(2, abuf[0..]) ?? unreachable;
    expect_eq(an, 4);
    const ack = parseTftpAck(abuf[0..]) ?? unreachable;
    expect_eq(ack.block, 2 as u16);
});

test("3. ERROR packet encode/decode across standard RFC 1350 / RFC 2347 error codes", () => {
    let ebuf: [64]u8 = std.mem.zeroes([64]u8);
    const en = encodeTftpError(ERR_FILE_NOT_FOUND, "No such file", ebuf[0..]) ?? unreachable;
    expect_eq(en, 4 + 12 + 1);
    expect_eq(ebuf[en - 1], 0 as u8);
    expect_eq(peekTftpOpcode(ebuf[0..en]) ?? 0, OP_ERROR);

    const err = parseTftpError(ebuf[0..en]) ?? unreachable;
    expect_eq(err.code, ERR_FILE_NOT_FOUND);
    expect_eq(std.mem.eql(u8, err.message, "No such file"), true);

    // Option negotiation error (8)
    const en2 = encodeTftpError(ERR_OPT_NEGOTIATION, "bad blksize", ebuf[0..]) ?? unreachable;
    const err2 = parseTftpError(ebuf[0..en2]) ?? unreachable;
    expect_eq(err2.code, ERR_OPT_NEGOTIATION);
});

test("4. OACK packet encode/decode via TftpOptWriter", () => {
    let obuf: [64]u8 = std.mem.zeroes([64]u8);
    let w = TftpOptWriter.initOack(obuf[0..]);
    expect_eq(w.writeOption("blksize", "1468"), true);
    expect_eq(w.writeOption("tsize", "65536"), true);
    const wire = w.finish() ?? unreachable;
    expect_eq(peekTftpOpcode(wire) ?? 0, OP_OACK);

    const oack = parseTftpOack(wire) ?? unreachable;
    expect_eq(std.mem.eql(u8, oack.findOption("BLKSIZE") ?? unreachable, "1468"), true);
    expect_eq(std.mem.eql(u8, oack.findOption("tsize") ?? unreachable, "65536"), true);
});

test("5. Missing NUL terminator, odd option pair, invalid ACK length, and invalid opcode rejection", () => {
    // 1. Invalid opcodes (0 and 7)
    const bad_op0: [4]u8 = [0, 0, 0, 1];
    const bad_op7: [4]u8 = [0, 7, 0, 1];
    expect_eq(peekTftpOpcode(bad_op0[0..]) == null, true);
    expect_eq(peekTftpOpcode(bad_op7[0..]) == null, true);
    expect_eq(isTftp(bad_op0[0..]), false);

    // 2. ACK with length != 4 (length 3 and length 5)
    const ack3: [3]u8 = [0, 4, 0];
    const ack5: [5]u8 = [0, 4, 0, 1, 0];
    expect_eq(parseTftpAck(ack3[0..]) == null, true);
    expect_eq(parseTftpAck(ack5[0..]) == null, true);
    expect_eq(isTftp(ack5[0..]), false);

    // 3. ERROR missing trailing NUL terminator
    const err_nonul: [7]u8 = [0, 5, 0, 1, 'b', 'a', 'd'];
    expect_eq(parseTftpError(err_nonul[0..]) == null, true);

    // 4. RRQ missing NUL on mode
    const rrq_nonul: [9]u8 = [0, 1, 'a', 0, 'o', 'c', 't', 'e', 't'];
    expect_eq(parseTftpRequest(rrq_nonul[0..]) == null, true);

    // 5. OACK with odd option (name\0 without value\0)
    const oack_odd: [10]u8 = [0, 6, 'b', 'l', 'k', 's', 'i', 'z', 'e', 0];
    expect_eq(parseTftpOack(oack_odd[0..]) == null, true);

    // 6. Insufficient buffer for encode helpers
    let tiny: [3]u8 = [0; 3];
    expect_eq(encodeTftpAck(1, tiny[0..]) == null, true);
    expect_eq(encodeTftpData(1, "", tiny[0..]) == null, true);
    expect_eq(encodeTftpError(0, "", tiny[0..]) == null, true);
    expect_eq(encodeTftpRequest(false, "a", "b", tiny[0..]) == null, true);
});
