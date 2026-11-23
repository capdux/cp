// Source: cp/cap/test/testSqlMySqlWireRustRefSuite.cp
// Reference-hardened against blackbeam/rust_mysql_common (MIT/Apache-2.0) & MySQL Client/Server Protocol
// Covers:
// 1. lenenc-int all 5 range boundaries (0, 250, 251, 65535, 65536, 16777215, 16777216, u64::MAX) + 0xFB NULL + 0xFF ERR
// 2. 4-byte packet framing, multi-packet concatenation, progressive partial feeding (atomic ShortRead), sequence_id & lenenc-str forged length guards
// 3. 16-lane SIMD vs scalar 0-discrepancy differential parity (simdMySqlFindNul, simdMySqlCopyBytes)
// 4. 1,500-case deterministic PRNG malformed MySQL wire & lenenc fuzz suite (0 panic / 0 OOB / 0 hang)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    MYSQL_LENENC_NULL_MARKER,
    MYSQL_LENENC_ERR_MARKER,
    MYSQL_LENENC_MAX_STR_LEN,
    encodedLengthIntSize,
    encodeLengthInt,
    decodeLengthIntStrict,
    encodeLengthString,
    decodeLengthString,
} from "../../lib/sql/mysqlLenenc.cp";
import {
    MYSQL_MAX_PACKET_LEN,
    MySqlWireReader,
    simdMySqlFindNul,
} from "../../lib/sql/mysqlReader.cp";
import {
    MySqlWireWriter,
    simdMySqlCopyBytes,
} from "../../lib/sql/mysqlWriter.cp";

// --- Scalar Reference Implementations for Differential Parity ---

fn scalarFindNul(s: []const u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == 0) return i;
    }
    return null;
}

struct Rng {
    state: u64;

    fn nextU64(*_): u64 {
        let x = _.state;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        _.state = x;
        return x *% 0x2545F4914F6CDD1D;
    }

    fn nextU8(*_): u8 {
        return ((_.nextU64() >> 32) & 0xFF) as u8;
    }

    fn nextRange(*_, bound: usize): usize {
        return ((_.nextU64() as usize) % bound);
    }
}

// --- Test 1: lenenc-int All 5 Range Boundaries + NULL/ERR + lenenc-str Roundtrip ---

fn testLenEncBoundariesAndRoundtrip(): !void {
    const vals: [8]u64 = [
        0 as u64,
        250 as u64,
        251 as u64,
        65535 as u64,
        65536 as u64,
        16777215 as u64,
        16777216 as u64,
        std.math.maxInt(u64),
    ];
    const expected_sizes: [8]usize = [1, 1, 3, 3, 4, 4, 9, 9];

    let idx: usize = 0;
    while (idx < vals.len) : (idx += 1) {
        const v = vals[idx];
        expect_eq(encodedLengthIntSize(v), expected_sizes[idx]);
        const enc = encodeLengthInt(v);
        expect_eq(enc.len, expected_sizes[idx]);
        const decoded = try decodeLengthIntStrict(enc.slice());
        expect_eq(decoded.value, v);
        expect_eq(decoded.bytes_read, expected_sizes[idx]);
    }

    // 0xFB NULL marker & 0xFF ERR marker rejection in strict mode
    const null_m: [1]u8 = [MYSQL_LENENC_NULL_MARKER];
    let got_null = false;
    * = decodeLengthIntStrict(&null_m) catch |e| {
        got_null = std.mem.eql(u8, @errorName(e), "NullMarker");
    };
    expect_eq(got_null, true);

    const err_m: [1]u8 = [MYSQL_LENENC_ERR_MARKER];
    let got_err = false;
    * = decodeLengthIntStrict(&err_m) catch |e| {
        got_err = std.mem.eql(u8, @errorName(e), "ErrMarker");
    };
    expect_eq(got_err, true);

    // Forged 8-byte huge lenenc-str (0xFE 0xFF..0xFF) MUST fail with LengthOverflow
    const forged_str: [9]u8 = [0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff];
    let ovf_rej = false;
    * = decodeLengthString(&forged_str) catch |e| {
        ovf_rej = std.mem.eql(u8, @errorName(e), "LengthOverflow");
    };
    expect_eq(ovf_rej, true);
    expect_eq(MYSQL_LENENC_MAX_STR_LEN, (16 * 1024 * 1024) as u64);

    // lenenc-str roundtrip
    let sbuf: [128]u8 = undefined;
    const payload = "mysql_lenenc_simd_payload_0123456789abcdef";
    const slen = try encodeLengthString(payload, &sbuf);
    const sdec = try decodeLengthString(sbuf[0..slen]);
    expect_eq(std.mem.eql(u8, sdec.data, payload), true);
    expect_eq(sdec.bytes_read, slen);
}

// --- Test 2: 4-Byte Packet Framing, Partial Feeding, Sequence & NUL Guards ---

fn testMySqlPacketsAndSecurityGuards(): !void {
    expect_eq(MYSQL_MAX_PACKET_LEN, 0xffffff as u32);

    let wbuf: [512]u8 = undefined;
    let w = MySqlWireWriter.init(&wbuf);

    // Packet 0: COM_QUERY with NUL-terminated string + lenenc-str + lenenc-NULL
    const p0 = try w.beginPacket(0);
    * = try w.writeByte(0x03);
    * = try w.writeNullTerminatedStrict("SELECT 1");
    * = try w.writeLenEncString("param_val");
    * = try w.writeLenEncNull();
    * = try w.finishPacket(p0);

    // Packet 1: Follow-up packet with sequence_id = 1
    const p1 = try w.beginPacket(1);
    * = try w.writeBytes("second_packet_body");
    * = try w.finishPacket(p1);

    const wire = w.written();

    // Progressive partial feeding on Packet 0 MUST return ShortRead and preserve pos==0
    let pkt0_total: usize = 0;
    {
        let probe = MySqlWireReader.init(wire);
        const pkt0 = try probe.readPacket();
        pkt0_total = (4 as usize) + pkt0.payload.len;
    }
    for (cut of 0..pkt0_total) {
        const cut_u = cut as usize;
        let rd_p = MySqlWireReader.init(wire[0..cut_u]);
        let is_short = false;
        * = rd_p.readPacket() catch |e| {
            is_short = std.mem.eql(u8, @errorName(e), "ShortRead");
        };
        expect_eq(is_short, true);
        expect_eq(rd_p.offset(), 0 as usize);
    }

    // Full 2-packet decode with sequence_id continuity verification
    let rd = MySqlWireReader.init(wire);
    const pkt0 = try rd.readPacketWithLimit(MYSQL_MAX_PACKET_LEN, 0);
    expect_eq(pkt0.sequence_id, 0 as u8);
    expect_eq(pkt0.is_continuation, false);
    let inner0 = MySqlWireReader.init(pkt0.payload);
    expect_eq(try inner0.readByte(), 0x03 as u8);
    expect_eq(std.mem.eql(u8, try inner0.readNullTerminated(), "SELECT 1"), true);
    expect_eq(std.mem.eql(u8, try inner0.readLenEncString(), "param_val"), true);
    expect_eq((try inner0.readLenEncStringOrNull()) == null, true);

    // Sequence mismatch guard (expecting 5 when actual is 1) MUST fail & keep pos atomic
    const saved_pos = rd.offset();
    let seq_rej = false;
    * = rd.readPacketWithLimit(MYSQL_MAX_PACKET_LEN, 5) catch |e| {
        seq_rej = std.mem.eql(u8, @errorName(e), "SequenceMismatch");
    };
    expect_eq(seq_rej, true);
    expect_eq(rd.offset(), saved_pos);

    const pkt1 = try rd.readPacketWithLimit(MYSQL_MAX_PACKET_LEN, 1);
    expect_eq(std.mem.eql(u8, pkt1.payload, "second_packet_body"), true);
    expect_eq(rd.remaining(), 0 as usize);

    // PacketTooLarge guard
    const big_pkt: [4]u8 = [0xFF, 0xFF, 0xFF, 0x00];
    let rd_big = MySqlWireReader.init(&big_pkt);
    let too_big = false;
    * = rd_big.readPacketWithLimit(1024, 0) catch |e| {
        too_big = std.mem.eql(u8, @errorName(e), "PacketTooLarge");
    };
    expect_eq(too_big, true);

    // Unterminated NUL string in reader & embedded NUL in writer
    let rd_unterm = MySqlWireReader.init("missing_nul");
    let unterm_rej = false;
    * = rd_unterm.readNullTerminated() catch |e| {
        unterm_rej = std.mem.eql(u8, @errorName(e), "UnterminatedString");
    };
    expect_eq(unterm_rej, true);

    let emb_buf: [64]u8 = undefined;
    let emb_w = MySqlWireWriter.init(&emb_buf);
    let emb_rej = false;
    * = emb_w.writeZ("bad\x00mid") catch |e| {
        emb_rej = std.mem.eql(u8, @errorName(e), "EmbeddedNul");
    };
    expect_eq(emb_rej, true);

    let strict_rej = false;
    * = emb_w.writeNullTerminatedStrict("bad\x00") catch |e| {
        strict_rej = std.mem.eql(u8, @errorName(e), "EmbeddedNul");
    };
    expect_eq(strict_rej, true);
}

// --- Test 3: 16-Lane SIMD vs Scalar Differential Parity (0 Discrepancy) ---

fn testSimdVsScalarParity(): !void {
    let rng = Rng { state: 0x39EF_42B1_7C8A_5D03 };
    let buf: [96]u8 = undefined;
    let dst: [96]u8 = undefined;

    for (len of 0..97) {
        const len_u = len as usize;

        // 1. simdMySqlFindNul parity
        for (i of 0..len) buf[i] = 'm';
        if (len_u > 0 && (len_u % 2) == 0) {
            buf[rng.nextRange(len_u)] = 0;
        }
        const s1 = buf[0..len_u];
        expect_eq(simdMySqlFindNul(s1), scalarFindNul(s1));

        // 2. simdMySqlCopyBytes parity
        for (i of 0..len) buf[i] = rng.nextU8();
        const copied = simdMySqlCopyBytes(dst[0..len_u], buf[0..len_u]);
        expect_eq(copied, len_u);
        expect_eq(std.mem.eql(u8, dst[0..len_u], buf[0..len_u]), true);
    }
}

// --- Test 4: 1,500-Case Deterministic PRNG Malformed MySQL Wire & lenenc Fuzz Suite ---

fn fuzzCheckMySql(slice: []const u8) {
    * = decodeLengthIntStrict(slice) catch {};
    * = decodeLengthString(slice) catch {};
    let rd1 = MySqlWireReader.init(slice);
    * = rd1.readPacket() catch {};
    let rd2 = MySqlWireReader.init(slice);
    * = rd2.readNullTerminated() catch {};
    let rd3 = MySqlWireReader.init(slice);
    * = rd3.readLenEncStringOrNull() catch {};
}

fn testDeterministicMySqlFuzz(): !void {
    let rng = Rng { state: 0x71D4_9C2A_5E08_B361 };
    let buf: [192]u8 = undefined;
    const seed_pkt: [14]u8 = [0x0A, 0x00, 0x00, 0x00, 0x03, 'S', 'E', 'L', 'E', 'C', 'T', ' ', '1', 0x00];

    let total: usize = 0;
    for (case_idx of 0..1500) {
        const strat = (case_idx as usize) % 3;
        let len: usize = 0;
        if (strat == 0) {
            len = rng.nextRange(160);
            for (i of 0..len) buf[i] = rng.nextU8();
        } else if (strat == 1) {
            len = (1 as usize) + rng.nextRange(seed_pkt.len);
            @memcpy(buf[0..len], seed_pkt[0..len]);
            const muts = (1 as usize) + rng.nextRange(4);
            let m: usize = 0;
            while (m < muts) : (m += 1) {
                buf[rng.nextRange(len)] = rng.nextU8();
            }
        } else {
            // High-marker lenenc stress (0xFB..0xFF)
            len = (1 as usize) + rng.nextRange(24);
            const markers: [5]u8 = [0xfb, 0xfc, 0xfd, 0xfe, 0xff];
            buf[0] = markers[rng.nextRange(5)];
            for (i of 1..len) buf[i] = rng.nextU8();
        }

        fuzzCheckMySql(buf[0..len]);
        total += 1;
    }
    expect_eq(total, 1500 as usize);
}

fn main() {
    test("1. lenenc-int All 5 Range Boundaries + NULL/ERR + lenenc-str Roundtrip", () => {
        try testLenEncBoundariesAndRoundtrip();
    });
    test("2. 4-Byte Packet Framing, Partial Feeding, Sequence & NUL Guards", () => {
        try testMySqlPacketsAndSecurityGuards();
    });
    test("3. 16-Lane SIMD vs Scalar Differential Parity (0 Discrepancy)", () => {
        try testSimdVsScalarParity();
    });
    test("4. 1,500-Case Deterministic PRNG Malformed MySQL Wire & lenenc Fuzz Suite", () => {
        try testDeterministicMySqlFuzz();
    });
}
