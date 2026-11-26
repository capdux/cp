import { test, expect_eq } from "cap:test";
import { RingQueueU64, ByteRing } from "../../packages/@cap/ring-queue/src/index.cp";

fn strEq(buf: []const u8, expected: string): bool {
    if (buf.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < buf.len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/ring-queue — Strict FIFO push, peek & pop order", () => {
        let storage: [4]u64 = undefined;
        let q: RingQueueU64 = RingQueueU64.new(4);
        expect_eq(q.isEmpty(), true);
        expect_eq(q.remaining(), 4);

        expect_eq(q.push(&storage, 10), true);
        expect_eq(q.push(&storage, 20), true);
        expect_eq(q.push(&storage, 30), true);
        expect_eq(q.len(), 3);
        expect_eq(q.remaining(), 1);

        let v: u64 = 0;
        expect_eq(q.peek(&storage, &v), true);
        expect_eq(v, 10);
        expect_eq(q.len(), 3);

        expect_eq(q.pop(&storage, &v), true);
        expect_eq(v, 10);
        expect_eq(q.pop(&storage, &v), true);
        expect_eq(v, 20);
        expect_eq(q.pop(&storage, &v), true);
        expect_eq(v, 30);
        expect_eq(q.isEmpty(), true);
    });

    test("2. @cap/ring-queue — Full queue rejects push & empty queue rejects pop/peek", () => {
        let storage: [2]u64 = undefined;
        let q: RingQueueU64 = RingQueueU64.new(2);
        let out: u64 = 999;

        expect_eq(q.pop(&storage, &out), false);
        expect_eq(q.peek(&storage, &out), false);

        expect_eq(q.push(&storage, 1), true);
        expect_eq(q.push(&storage, 2), true);
        expect_eq(q.isFull(), true);
        expect_eq(q.push(&storage, 3), false);
        expect_eq(q.len(), 2);
    });

    test("3. @cap/ring-queue — 100 continuous wrap-around push/pop cycles", () => {
        let storage: [5]u64 = undefined;
        let q: RingQueueU64 = RingQueueU64.new(5);

        let i: u64 = 0;
        while (i < 100) {
            expect_eq(q.push(&storage, i * 10 + 1), true);
            expect_eq(q.push(&storage, i * 10 + 2), true);
            let a: u64 = 0;
            let b: u64 = 0;
            expect_eq(q.pop(&storage, &a), true);
            expect_eq(q.pop(&storage, &b), true);
            expect_eq(a, i * 10 + 1);
            expect_eq(b, i * 10 + 2);
            i += 1;
        }
        expect_eq(q.isEmpty(), true);
    });

    test("4. @cap/ring-queue — forcePush evicts oldest element when full", () => {
        let storage: [3]u64 = undefined;
        let q: RingQueueU64 = RingQueueU64.new(3);

        expect_eq(q.forcePush(&storage, 100), false);
        expect_eq(q.forcePush(&storage, 200), false);
        expect_eq(q.forcePush(&storage, 300), false);
        // Full -> evicts 100
        expect_eq(q.forcePush(&storage, 400), true);
        // Full -> evicts 200
        expect_eq(q.forcePush(&storage, 500), true);
        expect_eq(q.len(), 3);

        let v: u64 = 0;
        expect_eq(q.pop(&storage, &v), true); expect_eq(v, 300);
        expect_eq(q.pop(&storage, &v), true); expect_eq(v, 400);
        expect_eq(q.pop(&storage, &v), true); expect_eq(v, 500);
        expect_eq(q.isEmpty(), true);
    });

    test("5. @cap/ring-queue — ByteRing bulk writeBytes & readBytes across wrap boundary", () => {
        let backing: [10]u8 = undefined;
        let br: ByteRing = ByteRing.new(10);

        // Write 7 bytes ("abcdefg"), read 5 ("abcde") -> head=5, tail=7, count=2 ("fg")
        expect_eq(br.writeBytes(&backing, "abcdefg"), 7);
        let tmp: [16]u8 = undefined;
        const r1 = br.readBytes(&backing, tmp[0..5]);
        expect_eq(r1, 5);
        expect_eq(strEq(tmp[0..r1], "abcde"), true);

        // Now write 8 bytes ("01234567") -> wraps around end of 10-byte buffer!
        expect_eq(br.writeBytes(&backing, "01234567"), 8);
        expect_eq(br.isFull(), true);
        expect_eq(br.writeBytes(&backing, "extra"), 0);

        // Read all 10 bytes across wrap boundary -> "fg01234567"
        const r2 = br.readBytes(&backing, &tmp);
        expect_eq(r2, 10);
        expect_eq(strEq(tmp[0..r2], "fg01234567"), true);
        expect_eq(br.isEmpty(), true);
    });

    test("6. @cap/ring-queue — clear() & zero-capacity boundary safety", () => {
        let storage: [4]u64 = undefined;
        let q: RingQueueU64 = RingQueueU64.new(4);
        q.push(&storage, 1);
        q.push(&storage, 2);
        q.clear();
        expect_eq(q.isEmpty(), true);
        expect_eq(q.remaining(), 4);

        let q0: RingQueueU64 = RingQueueU64.new(0);
        let out: u64 = 0;
        expect_eq(q0.isFull(), true);
        expect_eq(q0.push(&storage, 1), false);
        expect_eq(q0.forcePush(&storage, 1), false);
        expect_eq(q0.pop(&storage, &out), false);

        let bbuf: [4]u8 = undefined;
        let br0: ByteRing = ByteRing.new(0);
        expect_eq(br0.writeBytes(&bbuf, "hi"), 0);
        expect_eq(br0.readBytes(&bbuf, &bbuf), 0);
    });
}
