import { test, expect_eq } from "cap:test";
import { RingBufU8, RingBufI64, RB_U8_CAP, RB_I64_CAP } from "../../packages/@cap/ringbuf/src/index.cp";

fn eqBuf(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/ringbuf — RingBufU8 push, peek, pop & FIFO order", () => {
        let rb = RingBufU8.new();
        expect_eq(rb.isEmpty(), true);
        expect_eq(rb.capacity(), RB_U8_CAP);
        expect_eq(rb.push(65), true);
        expect_eq(rb.push(66), true);
        expect_eq(rb.len(), 2);
        expect_eq(rb.freeLen(), RB_U8_CAP - 2);
        expect_eq(rb.peek() ?? 0, 65);
        expect_eq(rb.pop() ?? 0, 65);
        expect_eq(rb.pop() ?? 0, 66);
        expect_eq(rb.pop() == null, true);
    });

    test("2. @cap/ringbuf — Wrap-around across 256-byte boundary via pushSlice/popSlice", () => {
        let rb = RingBufU8.new();
        let tmp: [200]u8 = undefined;
        let i: usize = 0;
        while (i < 200) { tmp[i] = 120; i = i + 1; }
        expect_eq(rb.pushSlice(tmp[0..200]), 200);
        expect_eq(rb.popSlice(tmp[0..180]), 180);
        expect_eq(rb.len(), 20);
        // Push 100 more bytes to wrap tail past 256
        expect_eq(rb.pushSlice("0123456789"), 10);
        expect_eq(rb.len(), 30);
        let drop: [20]u8 = undefined;
        expect_eq(rb.popSlice(drop[0..20]), 20);
        let out: [16]u8 = undefined;
        let got = rb.popSlice(out[0..16]);
        expect_eq(got, 10);
        expect_eq(eqBuf(out[0..got], got, "0123456789"), true);
    });

    test("3. @cap/ringbuf — Full capacity rejection & pushOverwrite eviction", () => {
        let rb = RingBufU8.new();
        let i: usize = 0;
        while (i < RB_U8_CAP) {
            expect_eq(rb.push((i & 255) as u8), true);
            i = i + 1;
        }
        expect_eq(rb.isFull(), true);
        expect_eq(rb.push(99), false);
        let evicted = rb.pushOverwrite(77) ?? 255;
        expect_eq(evicted, 0);
        expect_eq(rb.peek() ?? 255, 1);
    });

    test("4. @cap/ringbuf — RingBufI64 push, pop & wrap-around", () => {
        let rb = RingBufI64.new();
        expect_eq(rb.capacity(), RB_I64_CAP);
        let i: usize = 0;
        while (i < 50) {
            rb.push(i as i64);
            i = i + 1;
        }
        let j: usize = 0;
        while (j < 40) {
            expect_eq(rb.pop() ?? -1, j as i64);
            j = j + 1;
        }
        rb.push(1000);
        rb.push(2000);
        expect_eq(rb.len(), 12);
        expect_eq(rb.peek() ?? -1, 40);
    });

    test("5. @cap/ringbuf — RingBufI64 pushOverwrite rolling window", () => {
        let rb = RingBufI64.new();
        let i: usize = 0;
        while (i < RB_I64_CAP) {
            rb.push((i + 1) as i64);
            i = i + 1;
        }
        expect_eq(rb.pushOverwrite(999) ?? -1, 1);
        expect_eq(rb.pushOverwrite(1000) ?? -1, 2);
        expect_eq(rb.peek() ?? -1, 3);
        expect_eq(rb.len(), RB_I64_CAP);
    });

    test("6. @cap/ringbuf — clear resets state cleanly", () => {
        let rb = RingBufU8.new();
        rb.pushSlice("abcdef");
        rb.clear();
        expect_eq(rb.isEmpty(), true);
        expect_eq(rb.freeLen(), RB_U8_CAP);
        expect_eq(rb.peek() == null, true);
    });
}
