import {
    wcwidth,
    strDisplayWidth,
    sliceToDisplayWidth,
} from "cap:unicode";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("wcwidth and strDisplayWidth for ASCII, CJK, combining marks, and Emoji", () => {
    expect(wcwidth('A') == 1);
    expect(wcwidth(0x0301) == 0); // combining acute accent
    expect(wcwidth(0x4E2D) == 2); // '中'
    expect(wcwidth(0x1F600) == 2); // '😀'

    expect(strDisplayWidth("hello") == 5);
    expect(strDisplayWidth("中文") == 4);
    expect(strDisplayWidth("a中b") == 4);
    // "e" + combining acute accent U+0301 -> 1 column
    expect(strDisplayWidth("e\xcc\x81") == 1);
    expect(strDisplayWidth("hi\xf0\x9f\x98\x80") == 4);
});

test("sliceToDisplayWidth respects column budget without splitting CJK", () => {
    // "中"(2) + "文"(2) + "!"(1): budget 3 fits only "中"(2)
    expect(strEq(sliceToDisplayWidth("中文!", 3), "中"));
    expect(strEq(sliceToDisplayWidth("中文!", 4), "中文"));
    expect(strEq(sliceToDisplayWidth("中文!", 5), "中文!"));
    expect(strEq(sliceToDisplayWidth("中文!", 1), ""));
});
