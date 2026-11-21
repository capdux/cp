import { test, expect_eq } from "cap:test";
import { IntrusiveField, IntrusiveHeap } from "cap:intrusive_heap";

struct TimerNode {
    id: u32,
    deadline: u64,
    heap: IntrusiveField(TimerNode) = IntrusiveField(TimerNode).empty(),
}

fn lessTimer(ctx: u32, a: *const TimerNode!, b: *const TimerNode!): bool {
    * = ctx;
    return a.deadline < b.deadline;
}

type TimerHeap = IntrusiveHeap(TimerNode, u32, lessTimer);

test("1. Empty heap peek, deleteMin, count, findMax", () => {
    let h = TimerHeap.init(0);
    expect_eq(h.isEmpty(), true);
    expect_eq(h.count(), usize(0));
    expect_eq(h.peek() == null, true);
    expect_eq(h.findMax() == null, true);
    expect_eq(h.deleteMin() == null, true);
});

test("2. Insert and pop elements in strictly ascending order", () => {
    let n1 = TimerNode{ id: 1, deadline: 50 };
    let n2 = TimerNode{ id: 2, deadline: 10 };
    let n3 = TimerNode{ id: 3, deadline: 30 };
    let n4 = TimerNode{ id: 4, deadline: 20 };
    let n5 = TimerNode{ id: 5, deadline: 40 };

    let h = TimerHeap.init(0);
    h.insert(&n1);
    h.insert(&n2);
    h.insert(&n3);
    h.insert(&n4);
    h.insert(&n5);

    expect_eq(h.isEmpty(), false);
    expect_eq(h.count(), usize(5));
    expect_eq(h.peek().?.deadline, u64(10));
    expect_eq(h.findMax().?.deadline, u64(50));

    expect_eq(h.deleteMin().?.deadline, u64(10));
    expect_eq(h.deleteMin().?.deadline, u64(20));
    expect_eq(h.deleteMin().?.deadline, u64(30));
    expect_eq(h.deleteMin().?.deadline, u64(40));
    expect_eq(h.deleteMin().?.deadline, u64(50));
    expect_eq(h.deleteMin() == null, true);
    expect_eq(h.isEmpty(), true);
});

test("3. Arbitrary middle and root node removal", () => {
    let n1 = TimerNode{ id: 1, deadline: 100 };
    let n2 = TimerNode{ id: 2, deadline: 25 };
    let n3 = TimerNode{ id: 3, deadline: 75 };
    let n4 = TimerNode{ id: 4, deadline: 50 };

    let h = TimerHeap.init(0);
    h.insert(&n1);
    h.insert(&n2);
    h.insert(&n3);
    h.insert(&n4);

    // Remove non-root middle node
    h.remove(&n4);
    expect_eq(h.count(), usize(3));
    expect_eq(h.peek().?.deadline, u64(25));

    // Remove root node via remove()
    h.remove(&n2);
    expect_eq(h.count(), usize(2));
    expect_eq(h.peek().?.deadline, u64(75));

    expect_eq(h.deleteMin().?.deadline, u64(75));
    expect_eq(h.deleteMin().?.deadline, u64(100));
    expect_eq(h.isEmpty(), true);
});

test("4. Duplicate keys preserve all nodes cleanly", () => {
    let a = TimerNode{ id: 1, deadline: 42 };
    let b = TimerNode{ id: 2, deadline: 42 };
    let c = TimerNode{ id: 3, deadline: 42 };

    let h = TimerHeap.init(0);
    h.insert(&a);
    h.insert(&b);
    h.insert(&c);

    expect_eq(h.count(), usize(3));
    let popped: usize = 0;
    while (n := h.deleteMin()) {
        expect_eq(n.deadline, u64(42));
        popped += 1;
    }
    expect_eq(popped, usize(3));
    expect_eq(h.isEmpty(), true);
});

test("5. Re-inserting popped nodes across multiple cycles", () => {
    let nodes: [8]TimerNode = [
        TimerNode{ id: 0, deadline: 80 },
        TimerNode{ id: 1, deadline: 10 },
        TimerNode{ id: 2, deadline: 60 },
        TimerNode{ id: 3, deadline: 20 },
        TimerNode{ id: 4, deadline: 70 },
        TimerNode{ id: 5, deadline: 30 },
        TimerNode{ id: 6, deadline: 50 },
        TimerNode{ id: 7, deadline: 40 },
    ];
    let h = TimerHeap.init(0);
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        h.insert(&nodes[i]);
    }
    expect_eq(h.count(), usize(8));

    // Pop 3 smallest, bump their deadlines, re-insert
    let first = h.deleteMin().?;
    expect_eq(first.deadline, u64(10));
    first.deadline = 95;
    h.insert(first);

    let second = h.deleteMin().?;
    expect_eq(second.deadline, u64(20));
    second.deadline = 90;
    h.insert(second);

    expect_eq(h.count(), usize(8));
    expect_eq(h.peek().?.deadline, u64(30));
    expect_eq(h.findMax().?.deadline, u64(95));

    let prev: u64 = 0;
    let drained: usize = 0;
    while (cur := h.deleteMin()) {
        expect_eq(cur.deadline >= prev, true);
        prev = cur.deadline;
        drained += 1;
    }
    expect_eq(drained, usize(8));
});
