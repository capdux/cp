import { test, expect_eq } from "cap:test";
import { CssLayerRule, CssLayerName } from "cap:css";

fn sliceAt(items: []const []const u8, idx: usize): ([]const u8)? {
    if (idx >= items.len) {
        return null;
    }
    return items[idx];
}

fn findDelim(s: []const u8, open_pos: usize, ch: u8): usize? {
    if (open_pos >= s.len || s[open_pos] != ch) {
        return null;
    }
    return open_pos;
}

fn main() {
    test("optional slice return and cap:css LayerRule import", () => {
        const name = CssLayerName.parse("theme.base") catch unreachable;
        expect_eq(name.count(), 2);
        const first = name.at(0);
        expect_eq(first != null, true);
        expect_eq(name.at(5) == null, true);

        const arr: [2][]const u8 = ["a", "b"];
        expect_eq(sliceAt(arr[0..], 0) != null, true);
        expect_eq(sliceAt(arr[0..], 2) == null, true);
        expect_eq(findDelim("{x}", 0, '{') == 0, true);
        expect_eq(findDelim("{x}", 1, '{') == null, true);

        * = @sizeOf(CssLayerRule);
    });
}
