// Origin: Pure-CP Stdlib Test Suite (lib/mysql/stackReader.cp) | Created: 2026-11-22
// Production Deep Verification Suite for MySQL Bounded Wire Cursor (`StackReader`):
//   1. Basic read, peek, peekByte, ensureCapacity & saturating signed skip (forward & backward)
//   2. markMessageStart, setOffsetFromStart, rewindMessage & out-of-bounds offset rejection
//   3. readZ NUL-terminated strings & missing-NUL ShortRead guard
//   4. Little-endian integer decoding (u8, u16, u24, u32, u64)
//   5. MySQL Length-Encoded Integer (1/2/3/8-byte, 0xFB NULL, 0xFF invalid) & Length-Encoded String
//   6. readPacketFrame multi-packet framing & atomic rewind on truncated half-packet

import { test, expect_eq, expect_true } from "cap:test";
import {
    StackReader,
} from "../../lib/mysql/stackReader.cp";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

// ============================================================================
// Test 1: read, peek, ensureCapacity & saturating signed skip
// ============================================================================
test("stackReader: read, peek, ensureCapacity & saturating signed skip", () => {
    let r = StackReader.init("abcdefgh");
    expect_eq(r.position(), 0);
    expect_eq(r.remaining(), 8);
    expect_true(!r.isEof());
    expect_true(r.ensureCapacity(8));
    expect_true(!r.ensureCapacity(9));

    if (b0 := r.peekByte()) {
        expect_eq(b0, 'a');
    } else {
        expect_true(false);
    }

    const s1 = r.read(3) catch "";
    expect_true(strEq(s1, "abc"));
    expect_eq(r.position(), 3);
    expect_true(strEq(r.peek(), "defgh"));

    // Saturating negative skip beyond 0 clamps to 0
    r.skip(-100);
    expect_eq(r.position(), 0);

    // Saturating positive skip beyond len clamps to len
    r.skip(999);
    expect_eq(r.position(), 8);
    expect_true(r.isEof());
    expect_true(r.peekByte() == null);

    // Negative skip back by 2
    r.skip(-2);
    expect_true(strEq(r.peek(), "gh"));
});

// ============================================================================
// Test 2: markMessageStart, setOffsetFromStart & rewindMessage
// ============================================================================
test("stackReader: message bookmarking, relative seek & invalid offset guard", () => {
    let r = StackReader.initAt("0123456789", 2);
    expect_eq(r.messageStart(), 2);
    expect_eq(r.position(), 2);

    r.skip(4);
    expect_eq(r.position(), 6);
    r.rewindMessage();
    expect_eq(r.position(), 2);

    r.setOffsetFromStart(5) catch {
        expect_true(false);
    };
    expect_eq(r.position(), 7);

    // Setting relative offset past buffer end must fail
    let bad_off = false;
    r.setOffsetFromStart(99) catch {
        bad_off = true;
    };
    expect_true(bad_off);
});

// ============================================================================
// Test 3: readZ NUL-terminated string & missing-NUL ShortRead guard
// ============================================================================
test("stackReader: readZ parses NUL-terminated strings and guards missing NUL", () => {
    const raw: [12]u8 = ['8', '.', '0', '.', '3', '6', 0, 'o', 'k', 0, 'x', 'y'];
    let r = StackReader.init(raw[0..]);

    const ver = r.readZ() catch "";
    expect_true(strEq(ver, "8.0.36"));
    const tag = r.readZ() catch "";
    expect_true(strEq(tag, "ok"));

    // Remaining "xy" has no NUL terminator -> ShortRead
    let missing_nul = false;
    * = r.readZ() catch brk: {
        missing_nul = true;
        break :brk "";
    };
    expect_true(missing_nul);
});

// ============================================================================
// Test 4: Little-endian integer decoding (u8, u16, u24, u32, u64)
// ============================================================================
test("stackReader: little-endian u8/u16/u24/u32/u64 decoding", () => {
    const raw: [18]u8 = [
        0x7F,                                           // u8  = 0x7F
        0x34, 0x12,                                     // u16 = 0x1234
        0x56, 0x34, 0x12,                               // u24 = 0x123456
        0x78, 0x56, 0x34, 0x12,                         // u32 = 0x12345678
        0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, // u64 = 0x0102030405060708
    ];
    let r = StackReader.init(raw[0..]);

    expect_eq(r.readU8() catch 0, 0x7F);
    expect_eq(r.readU16Le() catch 0, 0x1234);
    expect_eq(r.readU24Le() catch 0, 0x123456);
    expect_eq(r.readU32Le() catch 0, 0x12345678);
    expect_eq(r.readU64Le() catch 0, 0x0102030405060708);
    expect_true(r.isEof());
});

// ============================================================================
// Test 5: MySQL Length-Encoded Integer & Length-Encoded String
// ============================================================================
test("stackReader: MySQL length-encoded integers, NULL/invalid sentinels & strings", () => {
    const raw: [24]u8 = [
        42,                                             // 1-byte: 42
        0xFC, 0x00, 0x04,                               // 2-byte: 1024
        0xFD, 0x01, 0x00, 0x01,                         // 3-byte: 65537
        0xFE, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, // 8-byte: 0x100000000
        0x05, 'm', 'y', 's', 'q', 'l',                  // len-enc str: "mysql"
        0xFB,                                           // NULL sentinel
    ];
    let r = StackReader.init(raw[0..]);

    expect_eq(r.readLenEncInt() catch 0, 42);
    expect_eq(r.readLenEncInt() catch 0, 1024);
    expect_eq(r.readLenEncInt() catch 0, 65537);
    expect_eq(r.readLenEncInt() catch 0, 0x100000000);

    const s = r.readLenEncString() catch "";
    expect_true(strEq(s, "mysql"));

    // 0xFB must fail with NullLenEncValue
    let got_null = false;
    * = r.readLenEncInt() catch brk: {
        got_null = true;
        break :brk 0;
    };
    expect_true(got_null);

    // 0xFF must fail with InvalidLenEncInt
    const bad_ff: [1]u8 = [0xFF];
    let r_bad = StackReader.init(bad_ff[0..]);
    let got_invalid = false;
    * = r_bad.readLenEncInt() catch brk: {
        got_invalid = true;
        break :brk 0;
    };
    expect_true(got_invalid);
});

// ============================================================================
// Test 6: readPacketFrame multi-packet framing & atomic half-packet rewind
// ============================================================================
test("stackReader: readPacketFrame parses frames and rewinds atomically on half-packet", () => {
    // Frame 1: len=3, seq=1, payload="OK!"
    // Frame 2 (truncated): header says len=5, seq=2, only 2 bytes present ("ab")
    const wire: [13]u8 = [
        3, 0, 0, 1, 'O', 'K', '!',
        5, 0, 0, 2, 'a', 'b',
    ];
    let r = StackReader.init(wire[0..]);

    if (f1 := r.readPacketFrame() catch null) {
        expect_eq(f1.payload_len, 3);
        expect_eq(f1.sequence_id, 1);
        expect_true(strEq(f1.payload, "OK!"));
    } else {
        expect_true(false);
    }
    expect_eq(r.position(), 7);

    // Second frame is incomplete -> must fail AND rewind offset back to 7
    let truncated = false;
    * = r.readPacketFrame() catch brk: {
        truncated = true;
        break :brk null;
    };
    expect_true(truncated);
    expect_eq(r.position(), 7);
    expect_eq(r.remaining(), 6);
});
