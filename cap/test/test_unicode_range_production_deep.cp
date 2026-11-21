import { test, expect } from "cap:test";
import { UnicodeRange } from "cap:unicode";

test("unicode_range: parse single, range, and wildcard forms", () => {
    const r1 = UnicodeRange.parse("U+0041") catch unreachable;
    expect(r1.isSingle()).toBeTrue();
    expect(r1.contains('A')).toBeTrue();
    expect(r1.contains('B')).toBeFalse();
    expect(r1.count()).toBe(1);

    const r2 = UnicodeRange.parse("u+0025-00ff") catch unreachable;
    expect(r2.start).toBe(0x25);
    expect(r2.end).toBe(0xFF);
    expect(r2.contains('A')).toBeTrue();

    const r3 = UnicodeRange.parse("U+4??") catch unreachable;
    expect(r3.start).toBe(0x400);
    expect(r3.end).toBe(0x4FF);
    expect(r3.count()).toBe(256);

    const r4 = UnicodeRange.parse("U+???") catch unreachable;
    expect(r4.start).toBe(0x000);
    expect(r4.end).toBe(0xFFF);
});

test("unicode_range: wildcard ? optimization on toCss, merge, and error rejection", () => {
    let buf: [32]u8 = undefined;

    const r1 = UnicodeRange.init(0x400, 0x4FF) catch unreachable;
    expect(r1.toCss(buf[0..]) catch unreachable).toBe("U+4??");

    const r2 = UnicodeRange.init(0x0, 0xFF) catch unreachable;
    expect(r2.toCss(buf[0..]) catch unreachable).toBe("U+??");

    const r3 = UnicodeRange.init(0x25, 0xFF) catch unreachable;
    expect(r3.toCss(buf[0..]) catch unreachable).toBe("U+25-FF");

    const a = UnicodeRange.init(0x00, 0x7F) catch unreachable;
    const b = UnicodeRange.init(0x80, 0xFF) catch unreachable;
    expect(a.adjacentOrOverlaps(b)).toBeTrue();
    const m = a.merge(b).?;
    expect(m.start).toBe(0x00);
    expect(m.end).toBe(0xFF);

    const bad1 = UnicodeRange.parse("U+100-50") catch null;
    expect(bad1 == null).toBeTrue();
    const bad2 = UnicodeRange.parse("U+200000") catch null;
    expect(bad2 == null).toBeTrue();
    const bad3 = UnicodeRange.parse("U+4?5") catch null;
    expect(bad3 == null).toBeTrue();
});
