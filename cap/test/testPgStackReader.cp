import {
    PgStackReaderError,
    PgWireFrameSlice,
    PgStackReader,
} from "cap:pg";
import { expect, test } from "cap:test";

fn sliceEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("pg stackReader: scalar big-endian reads and message length helper", fn () {
    const wire: [15]u8 = [
        0x7F,                   // u8 = 127
        0x01, 0x02,             // u16 BE = 258
        0xFF, 0xFE,             // i16 BE = -2
        0x12, 0x34, 0x56, 0x78, // u32 BE = 0x12345678
        0xFF, 0xFF, 0xFF, 0xFB, // i32 BE = -5
        0x00, 0x00,             // extra 2 bytes
    ];
    let r = PgStackReader.init(wire[0..]);
    expect(r.remaining()).toBe(15);
    expect(r.isEof()).toBe(false);
    expect(r.ensureLength(15)).toBe(true);
    expect(r.ensureCapacity(16)).toBe(false);

    expect(r.readU8() catch 0).toBe(127);
    expect(r.readU16Be() catch 0).toBe(258);
    expect(r.readI16Be() catch 0).toBe(-2);
    expect(r.readU32Be() catch 0).toBe(0x12345678);
    expect(r.readI32Be() catch 0).toBe(-5);
    expect(r.remaining()).toBe(2);

    // Saturating skip past end never panics
    r.skip(999);
    expect(r.isEof()).toBe(true);
    expect(r.remaining()).toBe(0);
    expect(r.peek().len).toBe(0);
});

test("pg stackReader: readZ zero-copy C-string reads and missing NUL short-read", fn () {
    const buf: [13]u8 = ['S', 'E', 'L', 'E', 'C', 'T', 0, 'o', 'k', 0, 'n', 'o', 'z'];
    let r = PgStackReader.init(buf[0..]);

    const s1 = r.readZ() catch "";
    expect(sliceEql(s1, "SELECT")).toBe(true);

    const s2 = r.readZ() catch "";
    expect(sliceEql(s2, "ok")).toBe(true);

    // Remaining "noz" has no NUL terminator -> ShortRead and cursor stays before "noz"
    let got_short = false;
    * = r.readZ() catch |e| {
        got_short = e == PgStackReaderError.ShortRead;
    };
    expect(got_short).toBe(true);
    expect(r.remaining()).toBe(3);
    expect(sliceEql(r.peek(), "noz")).toBe(true);
});

test("pg stackReader: markMessageStart and rewindMessage bookmarks", fn () {
    const buf: [8]u8 = [10, 20, 30, 40, 50, 60, 70, 80];
    let r = PgStackReader.init(buf[0..]);
    * = r.read(2) catch "";
    r.markMessageStart();
    expect(r.message_start).toBe(2);

    const mid = r.read(3) catch "";
    expect(mid.len).toBe(3);
    expect(mid[0]).toBe(30);
    expect(r.offset).toBe(5);

    r.rewindMessage();
    expect(r.offset).toBe(2);
    expect(r.readU8() catch 0).toBe(30);
});

test("pg stackReader: multi-frame backend stream decoding", fn () {
    // Frame 1: ParameterStatus ('S', len=13, "UTF8\0on\0" -> 9B payload? Wait: "app\0cp\0" = 7B -> len = 11)
    // Frame 2: ReadyForQuery ('Z', len=5, 'I')
    const stream: [18]u8 = [
        'S', 0, 0, 0, 11, 'a', 'p', 'p', 0, 'c', 'p', 0,
        'Z', 0, 0, 0, 5, 'I',
    ];
    let r = PgStackReader.init(stream[0..]);

    const f1 = r.readBackendFrame() catch PgWireFrameSlice{};
    expect(f1.tag).toBe('S');
    expect(f1.payload_len).toBe(7);

    // Nested PgStackReader over f1.payload to parse key/value C-strings
    let pr = PgStackReader.init(f1.payload);
    expect(sliceEql(pr.readZ() catch "", "app")).toBe(true);
    expect(sliceEql(pr.readZ() catch "", "cp")).toBe(true);
    expect(pr.isEof()).toBe(true);

    const f2 = r.readBackendFrame() catch PgWireFrameSlice{};
    expect(f2.tag).toBe('Z');
    expect(f2.payload_len).toBe(1);
    expect(f2.payload[0]).toBe('I');
    expect(r.isEof()).toBe(true);
});

test("pg stackReader: atomic rollback on partial backend frame", fn () {
    // Frame 1 complete ('1' ParseComplete, len=4, 0B payload) + partial Frame 2 ('C', len=10, only 3B arrived)
    const partial: [13]u8 = [
        '1', 0, 0, 0, 4,
        'C', 0, 0, 0, 10, 'S', 'E', 'L',
    ];
    let r = PgStackReader.init(partial[0..]);

    const f1 = r.readBackendFrame() catch PgWireFrameSlice{};
    expect(f1.tag).toBe('1');
    expect(f1.payload_len).toBe(0);
    expect(r.offset).toBe(5);

    // Second frame is short -> must roll back offset to 5
    let got_short = false;
    * = r.readBackendFrame() catch |e| {
        got_short = e == PgStackReaderError.ShortRead;
    };
    expect(got_short).toBe(true);
    expect(r.offset).toBe(5);
    expect(r.remaining()).toBe(8);
});

test("pg stackReader: rejects invalid message length (< 4) with rollback", fn () {
    const bad_len: [6]u8 = ['E', 0, 0, 0, 3, 0];
    let r = PgStackReader.init(bad_len[0..]);

    let got_invalid = false;
    * = r.readBackendFrame() catch |e| {
        got_invalid = e == PgStackReaderError.InvalidMessageLength;
    };
    expect(got_invalid).toBe(true);
    expect(r.offset).toBe(0);

    // Also test standalone readMessageLength
    let r2 = PgStackReader.init(bad_len[1..]);
    let got_invalid2 = false;
    * = r2.readMessageLength() catch |e| {
        got_invalid2 = e == PgStackReaderError.InvalidMessageLength;
    };
    expect(got_invalid2).toBe(true);
});
