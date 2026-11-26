// cap/test/testCapPriorityQueue.cp
// Comprehensive unit tests for @cap/priority-queue

import { test, expect_eq } from "cap:test";
import {
    IndexedMinHeap,
} from "../../packages/@cap/priority-queue/src/index.cp";

fn main() {
    test("1. @cap/priority-queue — Empty heap & out-of-bounds guards", () => {
        let pq = IndexedMinHeap.init();
        expect_eq(pq.isEmpty(), true);
        expect_eq(pq.len(), 0 as usize);
        expect_eq(pq.peek().valid, false);
        expect_eq(pq.pop().valid, false);
        expect_eq(pq.contains(0 as u16), false);
        expect_eq(pq.push(128 as u16, 10 as i64), false);
        expect_eq(pq.changePriority(5 as u16, 1 as i64), false);
        expect_eq(pq.remove(5 as u16), false);
    });

    test("2. @cap/priority-queue — push & pop strict ascending priority order", () => {
        let pq = IndexedMinHeap.init();
        expect_eq(pq.push(10 as u16, 50 as i64), true);
        expect_eq(pq.push(20 as u16, 10 as i64), true);
        expect_eq(pq.push(30 as u16, 30 as i64), true);
        expect_eq(pq.push(40 as u16, -5 as i64), true);
        expect_eq(pq.len(), 4 as usize);
        expect_eq(pq.peek().id, 40 as u16);
        expect_eq(pq.priorityOf(30 as u16), 30 as i64);

        const p1 = pq.pop();
        const p2 = pq.pop();
        const p3 = pq.pop();
        const p4 = pq.pop();
        expect_eq(p1.id, 40 as u16);
        expect_eq(p2.id, 20 as u16);
        expect_eq(p3.id, 30 as u16);
        expect_eq(p4.id, 10 as u16);
        expect_eq(pq.isEmpty(), true);
    });

    test("3. @cap/priority-queue — Duplicate id push updates priority in-place", () => {
        let pq = IndexedMinHeap.init();
        * = pq.push(1 as u16, 100 as i64);
        * = pq.push(2 as u16, 50 as i64);
        // Re-push id 1 with lower priority 10 -> should update in-place without growing size
        expect_eq(pq.push(1 as u16, 10 as i64), true);
        expect_eq(pq.len(), 2 as usize);
        expect_eq(pq.peek().id, 1 as u16);
        expect_eq(pq.priorityOf(1 as u16), 10 as i64);
    });

    test("4. @cap/priority-queue — changePriority decrease-key & increase-key", () => {
        let pq = IndexedMinHeap.init();
        * = pq.push(0 as u16, 10 as i64);
        * = pq.push(1 as u16, 20 as i64);
        * = pq.push(2 as u16, 30 as i64);
        * = pq.push(3 as u16, 40 as i64);

        // Decrease key of id 3 to 5 -> rises to root
        expect_eq(pq.changePriority(3 as u16, 5 as i64), true);
        expect_eq(pq.peek().id, 3 as u16);

        // Increase key of id 3 to 100 -> sinks to bottom
        expect_eq(pq.changePriority(3 as u16, 100 as i64), true);
        expect_eq(pq.peek().id, 0 as u16);

        expect_eq(pq.pop().id, 0 as u16);
        expect_eq(pq.pop().id, 1 as u16);
        expect_eq(pq.pop().id, 2 as u16);
        expect_eq(pq.pop().id, 3 as u16);
    });

    test("5. @cap/priority-queue — Arbitrary remove of root, middle & leaf nodes", () => {
        let pq = IndexedMinHeap.init();
        let i: u16 = 0;
        while (i < (10 as u16)) {
            * = pq.push(i, ((10 as u16) - i) as i64);
            i += 1 as u16;
        }
        // Remove root (id 9, pri 1), middle (id 5, pri 5), and leaf (id 0, pri 10)
        expect_eq(pq.remove(9 as u16), true);
        expect_eq(pq.remove(5 as u16), true);
        expect_eq(pq.remove(0 as u16), true);
        expect_eq(pq.contains(5 as u16), false);
        expect_eq(pq.len(), 7 as usize);

        let prev: i64 = -1000;
        while (!pq.isEmpty()) {
            const item = pq.pop();
            expect_eq(item.valid, true);
            expect_eq(item.priority >= prev, true);
            prev = item.priority;
        }
    });

    test("6. @cap/priority-queue — Simulated Dijkstra relaxation & clear reuse", () => {
        // 4-node graph: 0->1(4), 0->2(1), 2->1(2), 1->3(1), 2->3(5)
        // Shortest distances from 0: dist[0]=0, dist[2]=1, dist[1]=3, dist[3]=4
        let pq = IndexedMinHeap.init();
        * = pq.push(0 as u16, 0 as i64);
        * = pq.push(1 as u16, 999 as i64);
        * = pq.push(2 as u16, 999 as i64);
        * = pq.push(3 as u16, 999 as i64);

        const u0 = pq.pop();
        expect_eq(u0.id, 0 as u16);
        * = pq.changePriority(1 as u16, 4 as i64);
        * = pq.changePriority(2 as u16, 1 as i64);

        const u2 = pq.pop();
        expect_eq(u2.id, 2 as u16);
        expect_eq(u2.priority, 1 as i64);
        // Relax 2->1 (1+2=3 < 4) and 2->3 (1+5=6)
        * = pq.changePriority(1 as u16, 3 as i64);
        * = pq.changePriority(3 as u16, 6 as i64);

        const u1 = pq.pop();
        expect_eq(u1.id, 1 as u16);
        expect_eq(u1.priority, 3 as i64);
        // Relax 1->3 (3+1=4 < 6)
        * = pq.changePriority(3 as u16, 4 as i64);

        const u3 = pq.pop();
        expect_eq(u3.id, 3 as u16);
        expect_eq(u3.priority, 4 as i64);

        * = pq.push(7 as u16, 77 as i64);
        pq.clear();
        expect_eq(pq.isEmpty(), true);
        expect_eq(pq.contains(7 as u16), false);
    });
}
