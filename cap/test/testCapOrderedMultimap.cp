import { test, expect_eq } from "cap:test";
import {
    OMM_CAP,
    OrderedMultiMap,
} from "../../packages/ordered-multimap/src/index.cp";

fn main() {
    test("1. @cap/ordered-multimap — empty map state", () => {
        let m = OrderedMultiMap.new();
        expect_eq(m.len(), 0 as usize);
        expect_eq(m.keysLen(), 0 as usize);
        expect_eq(m.containsKey("a"), false);
        expect_eq(m.getFirst("a") == null, true);
        expect_eq(m.removeAll("a"), 0 as usize);
    });

    test("2. @cap/ordered-multimap — append preserves multiple values in insertion order", () => {
        let m = OrderedMultiMap.new();
        expect_eq(m.append("accept", "text/html"), true);
        expect_eq(m.append("host", "example.com"), true);
        expect_eq(m.append("accept", "application/json"), true);
        expect_eq(m.append("accept", "*/*"), true);

        expect_eq(m.len(), 4 as usize);
        expect_eq(m.keysLen(), 2 as usize);
        expect_eq(m.containsKey("accept"), true);
        expect_eq(m.containsKey("host"), true);
        expect_eq(m.getFirst("accept").?, "text/html");

        let buf: [8]string = undefined;
        const n = m.getAll("accept", buf[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(buf[0], "text/html");
        expect_eq(buf[1], "application/json");
        expect_eq(buf[2], "*/*");
    });

    test("3. @cap/ordered-multimap — insert replaces all existing values for key", () => {
        let m = OrderedMultiMap.new();
        * = m.append("k", "v1");
        * = m.append("other", "o1");
        * = m.append("k", "v2");
        * = m.append("k", "v3");

        expect_eq(m.insert("k", "only"), true);
        expect_eq(m.len(), 2 as usize);
        expect_eq(m.keysLen(), 2 as usize);
        expect_eq(m.getFirst("k").?, "only");

        let buf: [4]string = undefined;
        const n = m.getAll("k", buf[0..4]);
        expect_eq(n, 1 as usize);
        expect_eq(buf[0], "only");
        expect_eq(m.getFirst("other").?, "o1");
    });

    test("4. @cap/ordered-multimap — removeAll removes every value for key and preserves others", () => {
        let m = OrderedMultiMap.new();
        * = m.append("a", "1");
        * = m.append("b", "2");
        * = m.append("a", "3");
        * = m.append("c", "4");

        expect_eq(m.removeAll("a"), 2 as usize);
        expect_eq(m.containsKey("a"), false);
        expect_eq(m.getFirst("a") == null, true);
        expect_eq(m.len(), 2 as usize);
        expect_eq(m.keysLen(), 2 as usize);
        expect_eq(m.getFirst("b").?, "2");
        expect_eq(m.getFirst("c").?, "4");
    });

    test("5. @cap/ordered-multimap — getAll respects output slice capacity", () => {
        let m = OrderedMultiMap.new();
        * = m.append("tag", "t1");
        * = m.append("tag", "t2");
        * = m.append("tag", "t3");

        let small: [2]string = undefined;
        const n = m.getAll("tag", small[0..2]);
        expect_eq(n, 2 as usize);
        expect_eq(small[0], "t1");
        expect_eq(small[1], "t2");
    });

    test("6. @cap/ordered-multimap — capacity limit and replacement when full", () => {
        let m = OrderedMultiMap.new();
        let i: usize = 0;
        while (i < OMM_CAP) : (i += 1) {
            expect_eq(m.append("k", "v"), true);
        }
        expect_eq(m.len(), OMM_CAP);
        expect_eq(m.keysLen(), 1 as usize);
        expect_eq(m.append("extra", "x"), false);
        expect_eq(m.insert("extra", "x"), false);
        expect_eq(m.insert("k", "collapsed"), true);
        expect_eq(m.len(), 1 as usize);
        expect_eq(m.getFirst("k").?, "collapsed");
    });
}
