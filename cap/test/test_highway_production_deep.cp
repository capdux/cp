// Source: cp/cap/test/test_highway_production_deep.cp
// Deep Production Verification Suite for cap:highway & cap.highway
// Covers:
//   1. 0..129 byte length gradient parity against std.mem for indexOfByte, lastIndexOfByte, countByte, indexOfAnyByte
//   2. indexOfNeq, commonPrefixLen, indexOfSlice edge cases (empty, single-byte, long prefix, tail hit)
//   3. isAscii, isUtf8ValidFast, containsControlChars (pure ASCII, multi-byte UTF-8, invalid UTF-8 sequences)
//   4. toLowerAscii, toUpperAscii, replaceByte branchless accuracy across punctuation/digits/mixed case
//   5. `import cap from "cap"; cap.highway` dynamic mounting & std.testing.allocator zero leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    indexOfByte,
    lastIndexOfByte,
    indexOfAnyByte,
    countByte,
    indexOfNeq,
    commonPrefixLen,
    indexOfSlice,
    isAscii,
    isUtf8ValidFast,
    containsControlChars,
    toLowerAscii,
    toUpperAscii,
    replaceByte,
} from "cap:highway";

test("cap:highway - 0..129 byte gradient parity with std.mem (indexOfByte/lastIndexOfByte/countByte/indexOfAnyByte)") {
    const alloc = std.testing.allocator;
    const max_len: usize = 129;
    let buf = alloc.alloc(u8, max_len) catch unreachable;
    defer alloc.free(buf);

    let len: usize = 0;
    while (len <= max_len) {
        let i: usize = 0;
        while (i < len) {
            buf[i] = ('a' + ((i % 26) as u8));
            i += 1;
        }
        const slice = buf[0..len];

        // Test absent needle
        expect_eq(indexOfByte(slice, '~') == null, true);
        expect_eq(lastIndexOfByte(slice, '~') == null, true);
        expect_eq(countByte(slice, '~'), 0);

        // Test present needle at various positions (start, middle, end)
        if (len > 0) {
            const probe_positions: [3]usize = [0, len / 2, len - 1];
            let p_idx: usize = 0;
            while (p_idx < 3) {
                const pos = probe_positions[p_idx];
                const saved = buf[pos];
                buf[pos] = '#';

                let std_first: usize? = null;
                let std_last: usize? = null;
                let std_cnt: usize = 0;
                let std_any: usize? = null;
                zig {
                    std_first = std.mem.indexOfScalar(u8, slice, '#');
                    std_last = std.mem.lastIndexOfScalar(u8, slice, '#');
                    std_cnt = std.mem.count(u8, slice, "#");
                    std_any = std.mem.indexOfAny(u8, slice, "\r\n#");
                }

                expect_eq(indexOfByte(slice, '#'), std_first);
                expect_eq(lastIndexOfByte(slice, '#'), std_last);
                expect_eq(countByte(slice, '#'), std_cnt);
                expect_eq(indexOfAnyByte(slice, "\r\n#"), std_any);

                buf[pos] = saved;
                p_idx += 1;
            }
        }

        // Test multi-needle (5+ needles fallback path)
        if (len >= 10) {
            buf[len - 1] = '!';
            let std_any5: usize? = null;
            zig {
                std_any5 = std.mem.indexOfAny(u8, slice, "@$%^&!");
            }
            expect_eq(indexOfAnyByte(slice, "@$%^&!"), std_any5);
        }

        len += 1;
    }

    // Empty needles guard
    expect_eq(indexOfAnyByte("abcdef", "") == null, true);
}

test("cap:highway - indexOfNeq, commonPrefixLen & indexOfSlice boundary suite") {
    // 1. indexOfNeq & commonPrefixLen
    expect_eq(indexOfNeq("", "") == null, true);
    expect_eq(commonPrefixLen("", ""), 0);

    expect_eq(indexOfNeq("abc", "abcd"), 3);
    expect_eq(commonPrefixLen("abc", "abcd"), 3);

    const long_a = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_tail_A";
    const long_b = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_tail_B";
    expect_eq(indexOfNeq(long_a, long_a) == null, true);
    expect_eq(commonPrefixLen(long_a, long_a), long_a.len);
    expect_eq(indexOfNeq(long_a, long_b), 69);
    expect_eq(commonPrefixLen(long_a, long_b), 69);

    // 2. indexOfSlice boundaries: empty, single-byte, 2-byte, >32 byte haystack, tail match
    expect_eq(indexOfSlice("anything", ""), 0);
    expect_eq(indexOfSlice("", "a") == null, true);
    expect_eq(indexOfSlice("ab", "abc") == null, true);
    expect_eq(indexOfSlice("hello", "e"), 1);
    expect_eq(indexOfSlice("abcdef", "ef"), 4);

    const hay = "______________________________________________________________NEEDLE_END";
    expect_eq(indexOfSlice(hay, "NEEDLE"), 62);
    expect_eq(indexOfSlice(hay, "_END"), 68);
    expect_eq(indexOfSlice(hay, "MISSING") == null, true);
}

test("cap:highway - isAscii, isUtf8ValidFast & containsControlChars") {
    // Pure ASCII
    const ascii_long = "The quick brown fox jumps over the lazy dog 0123456789 !@#$%^&*()_+";
    expect_eq(isAscii(ascii_long), true);
    expect_eq(isUtf8ValidFast(ascii_long), true);
    expect_eq(containsControlChars(ascii_long), false);

    // Control characters across 32B/16B/tail boundaries
    const with_lf = "0123456789abcdef0123456789abcdef0123\n";
    expect_eq(isAscii(with_lf), true);
    expect_eq(containsControlChars(with_lf), true);

    const with_nul = "hello\x00world";
    expect_eq(containsControlChars(with_nul), true);

    // Valid multi-byte UTF-8 after long ASCII prefix
    const utf8_valid = "0123456789abcdef0123456789abcdef_你好世界_🚀";
    expect_eq(isAscii(utf8_valid), false);
    expect_eq(isUtf8ValidFast(utf8_valid), true);

    // Invalid UTF-8 sequences
    const utf8_bad1: [4]u8 = ['a', 0xFF, 'b', 'c'];
    const utf8_bad2: [36]u8 = [
        'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a',
        'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a',
        'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a',
        'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a',
        0xE4, 0xB8, 'x', 'y',
    ];
    expect_eq(isAscii(&utf8_bad1), false);
    expect_eq(isUtf8ValidFast(&utf8_bad1), false);
    expect_eq(isUtf8ValidFast(&utf8_bad2), false);
}

test("cap:highway - toLowerAscii, toUpperAscii & replaceByte branchless transforms") {
    const alloc = std.testing.allocator;
    const src = "AbC-123_XyZ!@#[]{}~09_HelloWorld_SIMD_Highway_Test_99!";
    const dst_low = alloc.alloc(u8, src.len) catch unreachable;
    defer alloc.free(dst_low);
    const dst_up = alloc.alloc(u8, src.len) catch unreachable;
    defer alloc.free(dst_up);

    toLowerAscii(dst_low, src);
    expect_eq(std.mem.eql(u8, dst_low, "abc-123_xyz!@#[]{}~09_helloworld_simd_highway_test_99!"), true);

    toUpperAscii(dst_up, src);
    expect_eq(std.mem.eql(u8, dst_up, "ABC-123_XYZ!@#[]{}~09_HELLOWORLD_SIMD_HIGHWAY_TEST_99!"), true);

    // replaceByte in-place over >32B + tail
    replaceByte(dst_low, '_', '-');
    expect_eq(countByte(dst_low, '_'), 0);
    expect_eq(std.mem.eql(u8, dst_low, "abc-123-xyz!@#[]{}~09-helloworld-simd-highway-test-99!"), true);
}

test("cap.highway - dynamic namespace mounting & allocator zero leak") {
    const alloc = std.testing.allocator;
    let data = alloc.alloc(u8, 73) catch unreachable;
    defer alloc.free(data);

    @memset(data, 'A');
    data[70] = 'Z';

    expect_eq(cap.highway.indexOfByte(data, 'Z'), 70);
    expect_eq(cap.highway.lastIndexOfByte(data, 'A'), 72);
    expect_eq(cap.highway.countByte(data, 'A'), 72);
    expect_eq(cap.highway.isAscii(data), true);
    expect_eq(cap.highway.isUtf8ValidFast(data), true);
    expect_eq(cap.highway.containsControlChars(data), false);

    cap.highway.replaceByte(data, 'Z', 'a');
    expect_eq(cap.highway.indexOfSlice(data, "AAaAA"), 68);
}
