import { test, expect_eq } from "cap:test";
import { PriorityQueue } from "cap:col";

fn main() {
    test("PriorityQueue basic", () => {
        let pq = PriorityQueue<i32>.init();
        defer pq.deinit();
        try pq.add(10);
        try pq.add(30);
        try pq.add(20);
        expect_eq(pq.len(), 3);
        expect_eq(pq.pop() == 10, true);
        expect_eq(pq.pop() == 20, true);
        expect_eq(pq.pop() == 30, true);
        expect_eq(pq.pop() == null, true);
    });
}
