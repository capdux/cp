import { test, expect } from "cap:test";
import { PgBackendTag, PgFrame, PgMessageReader } from "cap:pg";

test("pg_reader: nextFrame decodes CommandComplete and ReadyForQuery", () => {
    // Frame 1: 'C' + len=13 (4 + "SELECT 1\0"(9))
    // Frame 2: 'Z' + len=5 (4 + 'I'(1))
    const stream: [20]u8 = [
        'C', 0x00, 0x00, 0x00, 13,
        'S', 'E', 'L', 'E', 'C', 'T', ' ', '1', 0x00,
        'Z', 0x00, 0x00, 0x00, 5, 'I',
    ];
    let r = PgMessageReader.init(stream[0..]);

    const f1 = r.nextFrame() catch unreachable;
    expect(f1.tag == .CommandComplete).toBeTrue();
    let p1 = PgMessageReader.init(f1.payload);
    expect(p1.readCString() catch unreachable).toBe("SELECT 1");

    const f2 = r.nextFrame() catch unreachable;
    expect(f2.tag.isReady()).toBeTrue();
    expect(f2.payload.len).toBe(1);
    expect(f2.payload[0]).toBe('I');
    expect(r.isEof()).toBeTrue();
});

test("pg_reader: nullable fields (-1 => null), big-endian integers, and rewind", () => {
    // 2 cols: col0 = "ok" (len=2), col1 = NULL (len=-1), then i16=-42, i64=0x0102030405060708
    const buf: [20]u8 = [
        0x00, 0x00, 0x00, 0x02, 'o', 'k',
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xD6, // -42
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    ];
    let r = PgMessageReader.init(buf[0..]);
    expect(r.peekI32Be().?).toBe(2);

    const c0 = r.readNullableField() catch unreachable;
    expect(c0.?).toBe("ok");
    const c1 = r.readNullableField() catch unreachable;
    expect(c1 == null).toBeTrue();

    r.markMessageStart();
    expect(r.readI16Be() catch unreachable).toBe(-42);
    expect(r.readI64Be() catch unreachable).toBe(0x0102030405060708);
    expect(r.isEof()).toBeTrue();

    r.rewindToMessageStart();
    expect(r.remaining()).toBe(10);
});

test("pg_reader: invalid lengths and truncated frames rejected", () => {
    const bad_len: [5]u8 = ['Z', 0x00, 0x00, 0x00, 2]; // len < 4
    let r1 = PgMessageReader.init(bad_len[0..]);
    const f1 = r1.nextFrame() catch null;
    expect(f1 == null).toBeTrue();

    const trunc: [6]u8 = ['D', 0x00, 0x00, 0x00, 20, 0x00];
    let r2 = PgMessageReader.init(trunc[0..]);
    const f2 = r2.nextFrame() catch null;
    expect(f2 == null).toBeTrue();
});
