import { test, expect_eq } from "cap:test";
import { PriorityQueue } from "cap:col";
const std = @import("std");

fn main() {
    test("PriorityQueue basic", () => {
        let pq = PriorityQueue(i32).init(std.heap.page_allocator);
        defer pq.deinit();
        pq.add(10);
        pq.add(30);
        pq.add(20);
        expect_eq(pq.len(), 3);
        expect_eq(pq.pop() == 30, true);
        expect_eq(pq.pop() == 20, true);
        expect_eq(pq.pop() == 10, true);
    });
}
