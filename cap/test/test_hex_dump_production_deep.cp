import std from "std";
import { test, expect_eq } from "cap:test";
import {
    encodeHexLower,
    encodeHexUpper,
    decodeHexStrict,
    decodeHex,
    formatHexDumpLine,
    formatHexDump,
} from "cap:hex_dump";

test("1. encodeHexLower, encodeHexUpper, and decodeHexStrict round-trip", () => {
    const raw: [5]u8 = [0x00, 0x0A, 0x4F, 0xDE, 0xFF];
    let hex_buf: [16]u8 = std.mem.zeroes([16]u8);
    let dec_buf: [16]u8 = std.mem.zeroes([16]u8);

    const low = encodeHexLower(&raw, &hex_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, low, "000a4fdeff"), true);

    const up = encodeHexUpper(&raw, &hex_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, up, "000A4FDEFF"), true);

    const back1 = decodeHexStrict(low, &dec_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, back1, &raw), true);

    const back2 = decodeHexStrict("000a4FdeFF", &dec_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, back2, &raw), true);

    // 严格模式拒绝奇数长度、空格与非法字符
    expect_eq(decodeHexStrict("abc", &dec_buf) == null, true);
    expect_eq(decodeHexStrict("00 0a", &dec_buf) == null, true);
    expect_eq(decodeHexStrict("zz", &dec_buf) == null, true);
});

test("2. decodeHex ignores spaces, colons, hyphens, and newlines while rejecting malformed input", () => {
    let out: [16]u8 = std.mem.zeroes([16]u8);
    const mac = decodeHex("aa:BB:cc-dd 01\n02", &out) ?? unreachable;
    const expected: [6]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0x01, 0x02];
    expect_eq(std.mem.eql(u8, mac, &expected), true);

    // 奇数半字节与非法字符仍拒绝
    expect_eq(decodeHex("aa:bb:c", &out) == null, true);
    expect_eq(decodeHex("aa:gg", &out) == null, true);
});

test("3. formatHexDumpLine matches canonical hexdump -C layout exactly", () => {
    let out: [128]u8 = std.mem.zeroes([128]u8);
    const line = formatHexDumpLine("Hello World!", 0, &out) ?? unreachable;
    expect_eq(
        std.mem.eql(u8, line, "00000000  48 65 6c 6c 6f 20 57 6f  72 6c 64 21              |Hello World!|"),
        true
    );

    // 含不可打印字节与非零偏移量
    const bin: [4]u8 = [0x00, 0x1F, 0x41, 0x7F];
    const line2 = formatHexDumpLine(&bin, 0x0010, &out) ?? unreachable;
    expect_eq(
        std.mem.eql(u8, line2, "00000010  00 1f 41 7f                                       |..A.|"),
        true
    );
});

test("4. formatHexDump multi-line output across 16-byte boundary", () => {
    const payload = "0123456789ABCDEFxy";
    let out: [256]u8 = std.mem.zeroes([256]u8);
    const dump = formatHexDump(payload, &out) ?? unreachable;
    const expected =
        "00000000  30 31 32 33 34 35 36 37  38 39 41 42 43 44 45 46  |0123456789ABCDEF|\n"
        ++ "00000010  78 79                                             |xy|";
    expect_eq(std.mem.eql(u8, dump, expected), true);

    // 空输入返回空切片
    const empty = formatHexDump("", &out) ?? unreachable;
    expect_eq(empty.len, usize(0));
});

test("5. buffer-too-small safety guards for encode, decode, and hexdump", () => {
    let tiny: [3]u8 = std.mem.zeroes([3]u8);
    expect_eq(encodeHexLower("ab", &tiny) == null, true);
    expect_eq(encodeHexUpper("ab", &tiny) == null, true);
    expect_eq(decodeHexStrict("01020304", &tiny) == null, true);
    expect_eq(decodeHex("01 02 03 04", &tiny) == null, true);
    expect_eq(formatHexDumpLine("hi", 0, &tiny) == null, true);
    expect_eq(formatHexDump("hi", &tiny) == null, true);
});
