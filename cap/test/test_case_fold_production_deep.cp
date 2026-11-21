import { test, expect } from "cap:test";
import { FoldInfo, caseFold, caseFoldEqual } from "cap:text";

test("case_fold: ASCII, Latin, Greek, Cyrillic, Fullwidth, Deseret 1-to-1 folds", () => {
    expect(caseFold('A').codepoints[0]).toBe('a');
    expect(caseFold('Z').codepoints[0]).toBe('z');
    expect(caseFold('a').isUnchanged('a')).toBeTrue();
    expect(caseFold('0').isUnchanged('0')).toBeTrue();

    // Latin Extended & alternating ranges
    expect(caseFold(0x00C0).codepoints[0]).toBe(0x00E0);
    expect(caseFold(0x0100).codepoints[0]).toBe(0x0101);
    expect(caseFold(0x0101).codepoints[0]).toBe(0x0101);
    expect(caseFold(0x0102).codepoints[0]).toBe(0x0103);

    // Greek, Cyrillic, Fullwidth, Deseret, Ohm/Kelvin/Micro
    expect(caseFold(0x0391).codepoints[0]).toBe(0x03B1);
    expect(caseFold(0x0410).codepoints[0]).toBe(0x0430);
    expect(caseFold(0xFF21).codepoints[0]).toBe(0xFF41);
    expect(caseFold(0x10400).codepoints[0]).toBe(0x10428);
    expect(caseFold(0x2126).codepoints[0]).toBe(0x03C9);
    expect(caseFold(0x212A).codepoints[0]).toBe('k');
    expect(caseFold(0x00B5).codepoints[0]).toBe(0x03BC);
});

test("case_fold: 1-to-2 and 1-to-3 multi-codepoint expansions", () => {
    // ß -> "ss"
    const ss = caseFold(0x00DF);
    expect(ss.n_codepoints).toBe(2);
    expect(ss.codepoints[0]).toBe('s');
    expect(ss.codepoints[1]).toBe('s');

    // ﬁ -> "fi"
    const fi = caseFold(0xFB01);
    expect(fi.n_codepoints).toBe(2);
    expect(fi.codepoints[0]).toBe('f');
    expect(fi.codepoints[1]).toBe('i');

    // ﬃ -> "ffi"
    const ffi = caseFold(0xFB03);
    expect(ffi.n_codepoints).toBe(3);
    expect(ffi.codepoints[0]).toBe('f');
    expect(ffi.codepoints[1]).toBe('f');
    expect(ffi.codepoints[2]).toBe('i');
});

test("case_fold: zero-allocation caseFoldEqual across multi-codepoint expansions", () => {
    const a: [5]u21 = ['S', 'T', 'R', 'A', 0x00DF]; // "STRAß"
    const b: [6]u21 = ['s', 't', 'r', 'a', 's', 's']; // "strass"
    expect(caseFoldEqual(a[0..], b[0..])).toBeTrue();

    const c: [2]u21 = ['O', 0xFB03]; // "Oﬃ"
    const d: [4]u21 = ['o', 'f', 'f', 'i']; // "offi"
    expect(caseFoldEqual(c[0..], d[0..])).toBeTrue();

    const e: [3]u21 = ['o', 'f', 'f'];
    expect(caseFoldEqual(c[0..], e[0..])).toBeFalse();
});
