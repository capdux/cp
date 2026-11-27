import { test, expect_eq } from "cap:test";
import { YALLIST_CAP, YallistU64 } from "../../packages/yallist/src/index.cp";

fn main() {
    test("1. @cap/yallist — Empty list state and push/pop/unshift/shift", () => {
        let list: YallistU64 = YallistU64.new();
        expect_eq(list.isEmpty(), true);
        expect_eq(list.len(), 0);
        expect_eq(list.capacity(), YALLIST_CAP);
        expect_eq(list.headVal() == null, true);
        expect_eq(list.tailVal() == null, true);
        expect_eq(list.pop() == null, true);
        expect_eq(list.shift() == null, true);

        const h1: i32 = list.push(10);
        const h2: i32 = list.push(20);
        const h0: i32 = list.unshift(5);
        expect_eq(h1 >= 0 && h2 >= 0 && h0 >= 0, true);
        expect_eq(list.len(), 3);
        expect_eq(list.headVal().?, 5);
        expect_eq(list.tailVal().?, 20);

        expect_eq(list.shift().?, 5);
        expect_eq(list.pop().?, 20);
        expect_eq(list.pop().?, 10);
        expect_eq(list.isEmpty(), true);
    });

    test("2. @cap/yallist — get(idx) bidirectional lookup and bounds check", () => {
        let list: YallistU64 = YallistU64.new();
        * = list.push(100);
        * = list.push(200);
        * = list.push(300);
        * = list.push(400);
        * = list.push(500);

        expect_eq(list.get(0).?, 100);
        expect_eq(list.get(1).?, 200);
        expect_eq(list.get(2).?, 300);
        expect_eq(list.get(3).?, 400);
        expect_eq(list.get(4).?, 500);
        expect_eq(list.get(5) == null, true);
    });

    test("3. @cap/yallist — removeNode() O(1) unlink and slot recycling", () => {
        let list: YallistU64 = YallistU64.new();
        const h1: i32 = list.push(1);
        const h2: i32 = list.push(2);
        const h3: i32 = list.push(3);

        // Remove middle node
        expect_eq(list.removeNode(h2), true);
        // Double remove and invalid handles return false
        expect_eq(list.removeNode(h2), false);
        expect_eq(list.removeNode(-1), false);
        expect_eq(list.removeNode(64), false);

        expect_eq(list.len(), 2);
        expect_eq(list.headVal().?, 1);
        expect_eq(list.tailVal().?, 3);

        // Remove head and tail
        expect_eq(list.removeNode(h1), true);
        expect_eq(list.headVal().?, 3);
        expect_eq(list.tailVal().?, 3);
        expect_eq(list.removeNode(h3), true);
        expect_eq(list.isEmpty(), true);
    });

    test("4. @cap/yallist — unshiftNode() and pushNode() O(1) LRU promotion/demotion", () => {
        let list: YallistU64 = YallistU64.new();
        const h1: i32 = list.push(10);
        const h2: i32 = list.push(20);
        const h3: i32 = list.push(30);

        // Promote tail h3 to head: [30, 10, 20]
        expect_eq(list.unshiftNode(h3), true);
        expect_eq(list.headVal().?, 30);
        expect_eq(list.tailVal().?, 20);

        // Demote head h3 to tail: [10, 20, 30]
        expect_eq(list.pushNode(h3), true);
        expect_eq(list.headVal().?, 10);
        expect_eq(list.tailVal().?, 30);

        // Promote middle h2 to head: [20, 10, 30]
        expect_eq(list.unshiftNode(h2), true);
        // Demote middle h1 to tail: [20, 30, 10]
        expect_eq(list.pushNode(h1), true);

        let arr: [4]u64 = undefined;
        const n: usize = list.toArray(&arr);
        expect_eq(n, 3);
        expect_eq(arr[0], 20);
        expect_eq(arr[1], 30);
        expect_eq(arr[2], 10);
    });

    test("5. @cap/yallist — reverse() and toArray()", () => {
        let list: YallistU64 = YallistU64.new();
        * = list.push(1);
        * = list.push(2);
        * = list.push(3);
        * = list.push(4);

        list.reverse();
        expect_eq(list.headVal().?, 4);
        expect_eq(list.tailVal().?, 1);

        let arr: [4]u64 = undefined;
        const n: usize = list.toArray(&arr);
        expect_eq(n, 4);
        expect_eq(arr[0], 4);
        expect_eq(arr[1], 3);
        expect_eq(arr[2], 2);
        expect_eq(arr[3], 1);
    });

    test("6. @cap/yallist — 64-slot freelist capacity limit and reuse", () => {
        let list: YallistU64 = YallistU64.new();
        let i: usize = 0;
        while (i < YALLIST_CAP) : (i += 1) {
            const h: i32 = list.push((i + 1) as u64);
            expect_eq(h >= 0, true);
        }
        expect_eq(list.len(), YALLIST_CAP);
        // 65th push must fail with -1
        expect_eq(list.push(999), -1);
        expect_eq(list.unshift(999), -1);

        // Pop one and verify recycled slot can be pushed again
        expect_eq(list.pop().?, 64);
        const recycled: i32 = list.push(6400);
        expect_eq(recycled >= 0, true);
        expect_eq(list.tailVal().?, 6400);
        expect_eq(list.len(), YALLIST_CAP);
    });
}
