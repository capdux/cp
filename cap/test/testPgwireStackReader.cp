import { test, expect } from "cap:test";
import {
    PG_FRAME_HEADER_LEN,
    PG_DEFAULT_MAX_FRAME_BYTES,
    PgStackReaderError,
    PgWireFrameSlice,
    PgStackReader,
} from "cap:pgwire";

test("PgStackReader big-endian scalar readers & readZ", () => {
    // u8(0x7F), i16(-2 => 0xFFFE), u16(0x1234), i32(-1 => 0xFFFFFFFF),
    // u32(0x01020304), u64(0x0102030405060708), "pg\0"
    const raw: [24]u8 = [
        0x7F,
        0xFF, 0xFE,
        0x12, 0x34,
        0xFF, 0xFF, 0xFF, 0xFF,
        0x01, 0x02, 0x03, 0x04,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        'p', 'g', 0,
    ];
    let r = PgStackReader.init(raw[0..]);
    expect(r.remaining()).toBe(24);
    expect(r.isAtEnd()).toBe(false);

    expect(r.readU8()?).toBe(0x7F);
    expect(r.readI16Be()?).toBe(-2);
    expect(r.readU16Be()?).toBe(0x1234);
    expect(r.readI32Be()?).toBe(-1);
    expect(r.readU32Be()?).toBe(0x01020304);
    expect(r.readU64Be()?).toBe(0x0102030405060708);
    expect(r.readZ()?).toBe("pg");
    expect(r.isAtEnd()).toBe(true);
    expect(r.remaining()).toBe(0);
});

test("PgStackReader multi-frame sequential extraction (coalesced TCP packet)", () => {
    // Frame 1: ReadyForQuery 'Z', len=5, 'I'
    // Frame 2: CommandComplete 'C', len=7, "OK\0"
    // Frame 3: Sync 'S', len=4, empty payload
    const wire: [19]u8 = [
        'Z', 0, 0, 0, 5, 'I',
        'C', 0, 0, 0, 7, 'O', 'K', 0,
        'S', 0, 0, 0, 4,
    ];
    let r = PgStackReader.init(wire[0..]);

    const f1 = r.readFrame(PG_DEFAULT_MAX_FRAME_BYTES)?;
    expect(f1.tag).toBe('Z');
    expect(f1.msg_len).toBe(5);
    expect(f1.payloadLen()).toBe(1);
    expect(f1.frameLen()).toBe(6);
    expect(f1.payload[0]).toBe('I');

    const f2 = r.readFrame(PG_DEFAULT_MAX_FRAME_BYTES)?;
    expect(f2.tag).toBe('C');
    expect(f2.payloadLen()).toBe(3);
    expect(f2.payload[0]).toBe('O');
    expect(f2.payload[1]).toBe('K');
    expect(f2.payload[2]).toBe(0);

    const f3 = r.readFrame(PG_DEFAULT_MAX_FRAME_BYTES)?;
    expect(f3.tag).toBe('S');
    expect(f3.payloadLen()).toBe(0);
    expect(f3.frameLen()).toBe(PG_FRAME_HEADER_LEN);
    expect(r.isAtEnd()).toBe(true);
});

test("atomic short-read rollback preserves cursor at message_start", () => {
    // Frame 1 complete (6 bytes), Frame 2 truncated (declares len=10, only 2 payload bytes present)
    const partial: [13]u8 = [
        'Z', 0, 0, 0, 5, 'T',
        'C', 0, 0, 0, 10, 'O', 'K',
    ];
    let r = PgStackReader.init(partial[0..]);
    const f1 = r.readFrame(PG_DEFAULT_MAX_FRAME_BYTES)?;
    expect(f1.tag).toBe('Z');
    expect(r.offset).toBe(6);

    let got_short = false;
    * = r.readFrame(PG_DEFAULT_MAX_FRAME_BYTES) catch |e| {
        got_short = (e == PgStackReaderError.ShortRead);
        PgWireFrameSlice{}
    };
    expect(got_short).toBe(true);
    // Cursor atomically rolled back to offset 6!
    expect(r.offset).toBe(6);
    expect(r.message_start).toBe(6);
    expect(r.remaining()).toBe(7);
});

test("bookmarks, saturating skip, setOffsetFromStart, reset & readI64Be", () => {
    const data: [10]u8 = [0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 'A', 'B'];
    let r = PgStackReader.init(data[0..]);
    r.markMessageStart();
    expect(r.readI64Be()?).toBe(-2);
    expect(r.offset).toBe(8);

    r.rewindMessage();
    expect(r.offset).toBe(0);

    const new_off = r.setOffsetFromStart(8)?;
    expect(new_off).toBe(8);
    expect(r.readU8()?).toBe('A');

    // Saturating skip beyond buffer end
    r.skip(999);
    expect(r.isAtEnd()).toBe(true);
    expect(r.offset).toBe(10);

    // Out-of-bounds setOffsetFromStart
    let got_oob = false;
    * = r.setOffsetFromStart(11) catch |e| {
        got_oob = (e == PgStackReaderError.OffsetOutOfBounds);
        0
    };
    expect(got_oob).toBe(true);

    r.reset();
    expect(r.offset).toBe(0);
    expect(r.message_start).toBe(0);
});

test("error guards: InvalidLength, FrameTooLarge, readZ ShortRead", () => {
    // InvalidLength (msg_len = 3 < 4)
    const bad_len: [5]u8 = ['Z', 0, 0, 0, 3];
    let r1 = PgStackReader.init(bad_len[0..]);
    let got_inv = false;
    * = r1.readFrame(PG_DEFAULT_MAX_FRAME_BYTES) catch |e| {
        got_inv = (e == PgStackReaderError.InvalidLength);
        PgWireFrameSlice{}
    };
    expect(got_inv).toBe(true);
    expect(r1.offset).toBe(0);

    // FrameTooLarge (msg_len = 100 -> total 101 > max 32)
    const big: [8]u8 = ['D', 0, 0, 0, 100, 0, 0, 0];
    let r2 = PgStackReader.init(big[0..]);
    let got_big = false;
    * = r2.readFrame(32) catch |e| {
        got_big = (e == PgStackReaderError.FrameTooLarge);
        PgWireFrameSlice{}
    };
    expect(got_big).toBe(true);
    expect(r2.offset).toBe(0);

    // readZ without NUL terminator -> ShortRead & cursor unchanged
    const no_nul: [4]u8 = ['a', 'b', 'c', 'd'];
    let r3 = PgStackReader.init(no_nul[0..]);
    let got_z_short = false;
    * = r3.readZ() catch |e| {
        got_z_short = (e == PgStackReaderError.ShortRead);
        ""
    };
    expect(got_z_short).toBe(true);
    expect(r3.offset).toBe(0);
});
