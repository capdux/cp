import { test, expect_eq } from "cap:test";
import { DisjointSet, MAX_ELEMENTS } from "../../packages/@cap/disjoint-set/src/index.cp";

fn main() {
    test("init and component counts", () => {
        let dsu = DisjointSet.init(5);
        expect_eq(dsu.len(), 5);
        expect_eq(dsu.componentCount(), 5);
        expect_eq(dsu.find(0), 0);
        expect_eq(dsu.find(4), 4);
        expect_eq(dsu.find(5), -1);
        expect_eq(dsu.setSize(2), 1);
    });

    test("unite merges components and returns false on redundant union", () => {
        let dsu = DisjointSet.init(6);
        expect_eq(dsu.unite(0, 1), true);
        expect_eq(dsu.unite(1, 2), true);
        expect_eq(dsu.unite(0, 2), false);
        expect_eq(dsu.connected(0, 2), true);
        expect_eq(dsu.connected(0, 3), false);
        expect_eq(dsu.componentCount(), 4);
        expect_eq(dsu.setSize(0), 3);
        expect_eq(dsu.setSize(2), 3);
    });

    test("union by size attaches smaller tree under larger tree", () => {
        let dsu = DisjointSet.init(6);
        * = dsu.unite(0, 1);
        * = dsu.unite(0, 2);
        * = dsu.unite(3, 4);
        expect_eq(dsu.unite(3, 0), true);
        expect_eq(dsu.setSize(4), 5);
        expect_eq(dsu.componentCount(), 2);
        expect_eq(dsu.connected(4, 2), true);
    });

    test("path halving compresses deep chain and read-only find matches", () => {
        let dsu = DisjointSet.init(16);
        let i: usize = 0;
        while (i < 15) {
            expect_eq(dsu.unite(i, i + 1), true);
            i += 1;
        }
        expect_eq(dsu.componentCount(), 1);
        expect_eq(dsu.setSize(15), 16);
        let r0 = dsu.find(0);
        let r15 = dsu.find(15);
        expect_eq(r0, r15);
        expect_eq(dsu.findReadOnly(15), r0);
    });

    test("out-of-bounds operations fail safely", () => {
        let dsu = DisjointSet.init(4);
        expect_eq(dsu.unite(0, 4), false);
        expect_eq(dsu.unite(9, 1), false);
        expect_eq(dsu.connected(0, 99), false);
        expect_eq(dsu.setSize(4), 0);
        expect_eq(dsu.findReadOnly(4), -1);
    });

    test("reset and max capacity clamping", () => {
        let dsu = DisjointSet.init(500);
        expect_eq(dsu.len(), MAX_ELEMENTS);
        expect_eq(dsu.componentCount(), MAX_ELEMENTS);
        * = dsu.unite(0, 255);
        expect_eq(dsu.connected(0, 255), true);
        expect_eq(dsu.componentCount(), 255);
        dsu.reset(8);
        expect_eq(dsu.len(), 8);
        expect_eq(dsu.componentCount(), 8);
        expect_eq(dsu.connected(0, 1), false);
    });
}
