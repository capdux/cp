import { test, expect } from "cap:test";
import {
    UTF8_ACCEPT,
    UTF8_REJECT,
    REPLACEMENT_CHAR,
    Utf8Iterator,
    isValidUtf8,
    countCodepoints,
    isAscii,
    isDigit,
    isUpper,
    isLower,
    isAlphabetic,
    isAlphanumeric,
    isHex,
    isControl,
    isPrint,
    isWhitespace,
    toUpper,
    toLower,
} from "cap:utf8_dfa";

test("utf8_dfa: ascii iteration, peek, remaining, reset", () => {
    let it = Utf8Iterator.init("abc");
    expect(it.peek().?).toBe('a');
    expect(it.next().?).toBe('a');
    expect(it.i).toBe(1);
    expect(it.remaining().len).toBe(2);
    expect(it.next().?).toBe('b');
    expect(it.next().?).toBe('c');
    expect(it.peek() == null).toBeTrue();
    expect(it.next() == null).toBeTrue();
    it.reset();
    expect(it.i).toBe(0);
    expect(it.next().?).toBe('a');
});

test("utf8_dfa: multibyte CJK and 4-byte emoji decoding", () => {
    const text = "你好😀😅";
    expect(isValidUtf8(text)).toBeTrue();
    expect(countCodepoints(text)).toBe(4);

    let it = Utf8Iterator.init(text);
    expect(it.next().?).toBe(0x4F60); // 你
    expect(it.next().?).toBe(0x597D); // 好
    expect(it.next().?).toBe(0x1F600); // 😀
    expect(it.next().?).toBe(0x1F605); // 😅
    expect(it.next() == null).toBeTrue();
});

test("utf8_dfa: overlong, surrogate, and truncated sequences emit U+FFFD", () => {
    const overlong: [4]u8 = [0xF0, 0x80, 0x80, 0xAF];
    expect(isValidUtf8(overlong[0..])).toBeFalse();
    let it = Utf8Iterator.init(overlong[0..]);
    expect(it.next().?).toBe(REPLACEMENT_CHAR);
    expect(it.next().?).toBe(REPLACEMENT_CHAR);
    expect(it.next().?).toBe(REPLACEMENT_CHAR);
    expect(it.next() == null).toBeTrue();

    // UTF-16 surrogate U+D800 encoded in UTF-8: ED A0 80 (invalid)
    const surrogate: [3]u8 = [0xED, 0xA0, 0x80];
    expect(isValidUtf8(surrogate[0..])).toBeFalse();

    // Truncated 3-byte sequence at EOF
    const trunc: [2]u8 = [0xE4, 0xBD];
    expect(isValidUtf8(trunc[0..])).toBeFalse();
    let it2 = Utf8Iterator.init(trunc[0..]);
    expect(it2.next().?).toBe(REPLACEMENT_CHAR);
    expect(it2.next() == null).toBeTrue();
});

test("utf8_dfa: branchless ASCII predicates and case folding", () => {
    expect(isAscii('A')).toBeTrue();
    expect(isAscii(0x80)).toBeFalse();
    expect(isDigit('7')).toBeTrue();
    expect(isDigit('a')).toBeFalse();
    expect(isUpper('Z')).toBeTrue();
    expect(isLower('z')).toBeTrue();
    expect(isAlphabetic('m')).toBeTrue();
    expect(isAlphanumeric('9')).toBeTrue();
    expect(isHex('f')).toBeTrue();
    expect(isHex('g')).toBeFalse();
    expect(isControl(0x00)).toBeTrue();
    expect(isControl(0x7F)).toBeTrue();
    expect(isPrint(' ')).toBeTrue();
    expect(isPrint('\n')).toBeFalse();
    expect(isWhitespace('\t')).toBeTrue();
    expect(isWhitespace(' ')).toBeTrue();

    expect(toUpper('a')).toBe('A');
    expect(toUpper('A')).toBe('A');
    expect(toUpper('5')).toBe('5');
    expect(toLower('Z')).toBe('z');
    expect(toLower('z')).toBe('z');
    expect(toLower(0x4F60)).toBe(0x4F60);
});
