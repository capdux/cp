import { test, expect } from "cap:test";
import { CharFreq, NameMinifier, HEAD_CHAR_COUNT } from "cap:text";

test("char_freq: small and big scan paths produce consistent counts", () => {
    let f = CharFreq.init();
    f.scan("abc_09$Z", 1);
    expect(f.countOf('a')).toBe(1);
    expect(f.countOf('b')).toBe(1);
    expect(f.countOf('c')).toBe(1);
    expect(f.countOf('_')).toBe(1);
    expect(f.countOf('0')).toBe(1);
    expect(f.countOf('9')).toBe(1);
    expect(f.countOf('$')).toBe(1);
    expect(f.countOf('Z')).toBe(1);
    expect(f.countOf('!')).toBe(0);

    // Big path (>= 32 bytes)
    f.scan("zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz0123456789", 2);
    expect(f.countOf('z')).toBe(64);
    expect(f.countOf('0')).toBe(3);
    expect(f.countOf('9')).toBe(3);
});

test("char_freq: include merges histograms and compile prioritizes top chars", () => {
    let f1 = CharFreq.init();
    let f2 = CharFreq.init();
    f1.scan("qqqqqq", 1);
    f2.scan("qqqqmmmmm999999999", 1);
    f1.include(f2);
    expect(f1.countOf('q')).toBe(10);
    expect(f1.countOf('m')).toBe(5);
    expect(f1.countOf('9')).toBe(9);

    const minifier = f1.compile();
    let buf: [8]u8 = undefined;
    // Digits cannot start an identifier, so head[0]='q', head[1]='m', while tail[0]='q', tail[1]='9'
    expect(minifier.numberToName(0, buf[0..])).toBe("q");
    expect(minifier.numberToName(1, buf[0..])).toBe("m");
    expect(minifier.tail[0]).toBe('q');
    expect(minifier.tail[1]).toBe('9');
    expect(minifier.tail[2]).toBe('m');
});

test("char_freq: multi-character base-54/64 name progression", () => {
    const m = NameMinifier.default();
    let buf: [8]u8 = undefined;
    expect(m.numberToName(0, buf[0..])).toBe("a");
    expect(m.numberToName(25, buf[0..])).toBe("z");
    expect(m.numberToName(53, buf[0..])).toBe("$");
    expect(m.numberToName(HEAD_CHAR_COUNT, buf[0..])).toBe("aa");
    expect(m.numberToName(HEAD_CHAR_COUNT + 1, buf[0..])).toBe("ba");
});
