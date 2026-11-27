// cap/test/testCapMultimap.cp
// Comprehensive unit tests for @cap/multimap (Rust multimap parity)

import { test, expect_eq } from "cap:test";
import { MultiMapStr } from "../../packages/multimap/src/index.cp";

fn main() {
    test("1. @cap/multimap — multi-value insert, getFirst, getLast & getNth", () => {
        let m = MultiMapStr.init();
        expect_eq(m.isEmpty(), true);
        expect_eq(m.totalValues(), 0 as usize);

        expect_eq(m.insert("Set-Cookie", "a=1"), true);
        expect_eq(m.insert("Host", "capdux.org"), true);
        expect_eq(m.insert("Set-Cookie", "b=2"), true);
        expect_eq(m.insert("Set-Cookie", "c=3"), true);

        expect_eq(m.totalValues(), 4 as usize);
        expect_eq(m.getFirst("Set-Cookie").?, "a=1");
        expect_eq(m.getLast("Set-Cookie").?, "c=3");
        expect_eq(m.getNth("Set-Cookie", 0 as usize).?, "a=1");
        expect_eq(m.getNth("Set-Cookie", 1 as usize).?, "b=2");
        expect_eq(m.getNth("Set-Cookie", 2 as usize).?, "c=3");
        expect_eq(m.getNth("Set-Cookie", 3 as usize) == null, true);
        expect_eq(m.getFirst("Missing") == null, true);
    });

    test("2. @cap/multimap — countFor, containsKey & uniqueKeyCount", () => {
        let m = MultiMapStr.init();
        * = m.insert("tag", "rust");
        * = m.insert("tag", "cp");
        * = m.insert("lang", "zh");
        * = m.insert("tag", "zig");
        * = m.insert("lang", "en");

        expect_eq(m.countFor("tag"), 3 as usize);
        expect_eq(m.countFor("lang"), 2 as usize);
        expect_eq(m.countFor("none"), 0 as usize);
        expect_eq(m.containsKey("tag"), true);
        expect_eq(m.containsKey("none"), false);
        expect_eq(m.uniqueKeyCount(), 2 as usize);
    });

    test("3. @cap/multimap — removeKey bulk delete preserves remaining order", () => {
        let m = MultiMapStr.init();
        * = m.insert("k1", "v1");
        * = m.insert("drop", "d1");
        * = m.insert("k2", "v2");
        * = m.insert("drop", "d2");
        * = m.insert("k3", "v3");

        const removed = m.removeKey("drop");
        expect_eq(removed, 2 as usize);
        expect_eq(m.removeKey("drop"), 0 as usize);
        expect_eq(m.totalValues(), 3 as usize);
        expect_eq(m.containsKey("drop"), false);

        expect_eq(m.entryAt(0 as usize).?.key, "k1");
        expect_eq(m.entryAt(1 as usize).?.key, "k2");
        expect_eq(m.entryAt(2 as usize).?.key, "k3");
    });

    test("4. @cap/multimap — removePair deletes only first matching (key, value)", () => {
        let m = MultiMapStr.init();
        * = m.insert("ev", "onClick");
        * = m.insert("ev", "onHover");
        * = m.insert("ev", "onClick");

        expect_eq(m.removePair("ev", "onMissing"), false);
        expect_eq(m.removePair("ev", "onClick"), true);
        expect_eq(m.countFor("ev"), 2 as usize);
        expect_eq(m.getFirst("ev").?, "onHover");
        expect_eq(m.getLast("ev").?, "onClick");
    });

    test("5. @cap/multimap — clear resets map cleanly", () => {
        let m = MultiMapStr.init();
        * = m.insert("a", "1");
        * = m.insert("b", "2");
        m.clear();
        expect_eq(m.isEmpty(), true);
        expect_eq(m.totalValues(), 0 as usize);
        expect_eq(m.uniqueKeyCount(), 0 as usize);
        expect_eq(m.getFirst("a") == null, true);
    });

    test("6. @cap/multimap — full capacity (128) boundary guard", () => {
        let m = MultiMapStr.init();
        let i: usize = 0;
        while (i < 128) {
            expect_eq(m.insert("k", "v"), true);
            i += 1;
        }
        expect_eq(m.isFull(), true);
        expect_eq(m.totalValues(), 128 as usize);
        expect_eq(m.uniqueKeyCount(), 1 as usize);
        expect_eq(m.insert("k", "overflow"), false);
        expect_eq(m.entryAt(128 as usize) == null, true);
    });
}
