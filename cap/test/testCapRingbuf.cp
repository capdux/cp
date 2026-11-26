// cap/test/testCapRingbuf.cp
// Comprehensive unit tests for @cap/ringbuf

import { test, expect_eq } from "cap:test";
import {
    RING_DEQUE_CAP,
    BYTE_RING_CAP,
    RingDequeI64,
    ByteRingBuf,
} from "../../packages/@cap/ringbuf/src/index.cp";

fn main() {
    test("1. @cap/ringbuf — RingDequeI64 FIFO & LIFO double-ended operations", () => {
        let dq = RingDequeI64.init();
        expect_eq(dq.isEmpty(), true);
        expect_eq(dq.capacity(), RING_DEQUE_CAP);
        expect_eq(dq.front() == null, true);
        expect_eq(dq.back() == null, true);

        expect_eq(dq.pushBack(10), true);
        expect_eq(dq.pushBack(20), true);
        expect_eq(dq.pushFront(5), true);
        // Order: [5, 10, 20]
        expect_eq(dq.len(), 3 as usize);
        expect_eq(dq.front() ?? 0, 5 as i64);
        expect_eq(dq.back() ?? 0, 20 as i64);
        expect_eq(dq.get(1) ?? 0, 10 as i64);
        expect_eq(dq.get(3) == null, true);

        expect_eq(dq.popFront() ?? 0, 5 as i64);
        expect_eq(dq.popBack() ?? 0, 20 as i64);
        expect_eq(dq.popFront() ?? 0, 10 as i64);
        expect_eq(dq.popFront() == null, true);
        expect_eq(dq.isEmpty(), true);
    });

    test("2. @cap/ringbuf — RingDequeI64 wrap-around across capacity boundary", () => {
        let dq = RingDequeI64.init();
        // Push and pop 100 times to wrap head/tail around 64 multiple times
        let i: i64 = 0;
        while (i < 100) {
            expect_eq(dq.pushBack(i), true);
            if (i >= 10) {
                expect_eq(dq.popFront() ?? -1, i - 10);
            }
            i += 1;
        }
        expect_eq(dq.len(), 10 as usize);
        expect_eq(dq.front() ?? -1, 90 as i64);
        expect_eq(dq.back() ?? -1, 99 as i64);
    });

    test("3. @cap/ringbuf — RingDequeI64 full rejection & pushBackOverwrite sliding window", () => {
        let dq = RingDequeI64.init();
        let i: i64 = 0;
        while (i < 64) {
            expect_eq(dq.pushBack(i), true);
            i += 1;
        }
        expect_eq(dq.isFull(), true);
        expect_eq(dq.pushBack(999), false);
        expect_eq(dq.pushFront(999), false);

        // Overwrite evicts 0, then 1
        expect_eq(dq.pushBackOverwrite(100) ?? -1, 0 as i64);
        expect_eq(dq.pushBackOverwrite(101) ?? -1, 1 as i64);
        expect_eq(dq.len(), 64 as usize);
        expect_eq(dq.front() ?? -1, 2 as i64);
        expect_eq(dq.back() ?? -1, 101 as i64);

        dq.clear();
        expect_eq(dq.isEmpty(), true);
    });

    test("4. @cap/ringbuf — ByteRingBuf basic byte & slice I/O with peek", () => {
        let rb = ByteRingBuf.init();
        expect_eq(rb.isEmpty(), true);
        expect_eq(rb.freeSpace(), BYTE_RING_CAP);

        expect_eq(rb.writeBytes("Hello, "), 7 as usize);
        expect_eq(rb.writeByte('C'), true);
        expect_eq(rb.writeByte('P'), true);
        expect_eq(rb.len(), 9 as usize);
        expect_eq(rb.peekByte(0) ?? 0, 'H' as u8);
        expect_eq(rb.peekByte(8) ?? 0, 'P' as u8);
        expect_eq(rb.peekByte(9) == null, true);

        let out: [32]u8 = undefined;
        expect_eq(rb.readBytes(out[0..5]), "Hello");
        expect_eq(rb.readByte() ?? 0, ',' as u8);
        expect_eq(rb.readBytes(out[0..32]), " CP");
        expect_eq(rb.isEmpty(), true);
    });

    test("5. @cap/ringbuf — ByteRingBuf seamless wrap-around across 512-byte tail", () => {
        let rb = ByteRingBuf.init();
        let scratch: [400]u8 = undefined;

        // Advance head to 480
        let k: usize = 0;
        while (k < 480) {
            expect_eq(rb.writeByte('x'), true);
            k += 1;
        }
        expect_eq(rb.readBytes(scratch[0..400]).len, 400 as usize);
        expect_eq(rb.readBytes(scratch[0..80]).len, 80 as usize);
        expect_eq(rb.isEmpty(), true);

        // Now write 60 bytes spanning indices 480..511 and 0..27
        const payload = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx";
        expect_eq(rb.writeBytes(payload), 60 as usize);
        expect_eq(rb.len(), 60 as usize);

        let out: [64]u8 = undefined;
        expect_eq(rb.readBytes(out[0..64]), payload);
        expect_eq(rb.isEmpty(), true);
    });

    test("6. @cap/ringbuf — ByteRingBuf full saturation & partial write clamping", () => {
        let rb = ByteRingBuf.init();
        let i: usize = 0;
        while (i < 508) {
            expect_eq(rb.writeByte('a'), true);
            i += 1;
        }
        expect_eq(rb.freeSpace(), 4 as usize);
        // Only 4 of 10 bytes fit
        expect_eq(rb.writeBytes("0123456789"), 4 as usize);
        expect_eq(rb.isFull(), true);
        expect_eq(rb.writeByte('z'), false);

        rb.clear();
        expect_eq(rb.freeSpace(), BYTE_RING_CAP);
        expect_eq(rb.readByte() == null, true);
    });
}
