import { BraceExpandIter } from "cap:shell";
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

test("BraceExpandIter expands comma lists, padded int ranges, stepped ranges, and alpha ranges", () => {
    let buf: [64]u8 = [0; 64];

    let it1 = BraceExpandIter.init("src/{a,b,c}.cp");
    expect(strEq(it1.next(buf[0..]).?, "src/a.cp"));
    expect(strEq(it1.next(buf[0..]).?, "src/b.cp"));
    expect(strEq(it1.next(buf[0..]).?, "src/c.cp"));
    expect(it1.next(buf[0..]) == null);

    let it2 = BraceExpandIter.init("img_{01..03}.png");
    expect(strEq(it2.next(buf[0..]).?, "img_01.png"));
    expect(strEq(it2.next(buf[0..]).?, "img_02.png"));
    expect(strEq(it2.next(buf[0..]).?, "img_03.png"));
    expect(it2.next(buf[0..]) == null);

    let it3 = BraceExpandIter.init("{6..2..2}");
    expect(strEq(it3.next(buf[0..]).?, "6"));
    expect(strEq(it3.next(buf[0..]).?, "4"));
    expect(strEq(it3.next(buf[0..]).?, "2"));
    expect(it3.next(buf[0..]) == null);

    let it4 = BraceExpandIter.init("k_{a..c}");
    expect(strEq(it4.next(buf[0..]).?, "k_a"));
    expect(strEq(it4.next(buf[0..]).?, "k_b"));
    expect(strEq(it4.next(buf[0..]).?, "k_c"));
    expect(it4.next(buf[0..]) == null);

    // Non-expandable literal yields once unchanged
    let it5 = BraceExpandIter.init("plain_{single}_file");
    expect(strEq(it5.next(buf[0..]).?, "plain_{single}_file"));
    expect(it5.next(buf[0..]) == null);
});
