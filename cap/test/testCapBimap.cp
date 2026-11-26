import { test, expect_eq } from "cap:test";
import { BiMap, MAX_PAIRS } from "../../packages/@cap/bimap/src/index.cp";

fn main() {
    test("empty bimap state and lookups", () => {
        let bm = BiMap.init();
        expect_eq(bm.len(), 0);
        expect_eq(bm.isEmpty(), true);
        expect_eq(bm.capacity(), MAX_PAIRS);
        expect_eq(bm.containsLeft(1), false);
        expect_eq(bm.containsRight(100), false);
        expect_eq(bm.getByLeft(1), -1);
        expect_eq(bm.getByRight(100), -1);
    });

    test("insert and bidirectional lookups", () => {
        let bm = BiMap.init();
        expect_eq(bm.insert(1, 100), true);
        expect_eq(bm.insert(2, 200), true);
        expect_eq(bm.insert(3, 300), true);
        expect_eq(bm.len(), 3);
        expect_eq(bm.getByLeft(2), 200);
        expect_eq(bm.getByRight(200), 2);
        expect_eq(bm.containsLeft(3), true);
        expect_eq(bm.containsRight(300), true);
    });

    test("overwrite preserves strict bijection on left, right, and cross conflicts", () => {
        let bm = BiMap.init();
        expect_eq(bm.insert(1, 10), true);
        expect_eq(bm.insert(2, 20), true);
        // Cross-conflict: left=1 and right=20 belong to different pairs -> merges into (1, 20)
        expect_eq(bm.insert(1, 20), true);
        expect_eq(bm.len(), 1);
        expect_eq(bm.getByLeft(1), 20);
        expect_eq(bm.getByRight(20), 1);
        expect_eq(bm.containsLeft(2), false);
        expect_eq(bm.containsRight(10), false);
    });

    test("insertNoOverwrite rejects any duplicate left or right", () => {
        let bm = BiMap.init();
        expect_eq(bm.insertNoOverwrite(10, 1000), true);
        expect_eq(bm.insertNoOverwrite(10, 2000), false);
        expect_eq(bm.insertNoOverwrite(20, 1000), false);
        expect_eq(bm.insertNoOverwrite(20, 2000), true);
        expect_eq(bm.len(), 2);
        expect_eq(bm.getByLeft(10), 1000);
        expect_eq(bm.getByRight(2000), 20);
    });

    test("removeByLeft and removeByRight maintain consistency", () => {
        let bm = BiMap.init();
        * = bm.insert(1, 101);
        * = bm.insert(2, 102);
        * = bm.insert(3, 103);
        expect_eq(bm.removeByLeft(2), true);
        expect_eq(bm.removeByLeft(2), false);
        expect_eq(bm.containsRight(102), false);
        expect_eq(bm.removeByRight(103), true);
        expect_eq(bm.containsLeft(3), false);
        expect_eq(bm.len(), 1);
        expect_eq(bm.getByLeft(1), 101);
    });

    test("capacity limit and clear", () => {
        let bm = BiMap.init();
        let i: u64 = 0;
        while (i < (MAX_PAIRS as u64)) {
            expect_eq(bm.insert(i, i + 1000), true);
            i += 1;
        }
        expect_eq(bm.len(), MAX_PAIRS);
        expect_eq(bm.insert(9999, 8888), false);
        bm.clear();
        expect_eq(bm.isEmpty(), true);
        expect_eq(bm.insert(9999, 8888), true);
        expect_eq(bm.getByRight(8888), 9999);
    });
}
