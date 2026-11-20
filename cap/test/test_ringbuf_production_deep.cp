// Source: cp/cap/test/test_ringbuf_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:ringbuf & cap.ringbuf

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    RingError,
    OverflowPolicy,
    RingBuffer,
    ByteRing,
    MicroBatcher,
} from "cap:ringbuf";

fn testRingBufferRejectWrapAroundAndBatch(): ! {
    const alloc = std.heap.page_allocator;
    let rb = RingBuffer(i64).init(alloc, 4, OverflowPolicy.Reject)?;
    defer rb.deinit();

    expect_eq(rb.isEmpty(), true);
    expect_eq(rb.isFull(), false);
    expect_eq(rb.capacity(), 4 as usize);

    rb.push(10)?;
    rb.push(20)?;
    rb.push(30)?;
    rb.push(40)?;
    expect_eq(rb.isFull(), true);
    expect_eq(rb.len(), 4 as usize);

    // Reject policy must return RingError.BufferFull when full
    let got_full = false;
    rb.push(50) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "BufferFull")) { got_full = true; }
    };
    expect_eq(got_full, true);

    // Pop 2 items so head advances to index 2, then push 2 more to wrap around tail
    expect_eq(rb.pop().?, 10 as i64);
    expect_eq(rb.pop().?, 20 as i64);
    rb.push(50)?;
    rb.push(60)?;
    expect_eq(rb.isFull(), true);

    // Logical peek and peekAt across wrap-around boundary: [30, 40, 50, 60]
    expect_eq(rb.peek().?, 30 as i64);
    expect_eq(rb.peekAt(0).?, 30 as i64);
    expect_eq(rb.peekAt(1).?, 40 as i64);
    expect_eq(rb.peekAt(2).?, 50 as i64);
    expect_eq(rb.peekAt(3).?, 60 as i64);
    expect_eq(rb.peekAt(4) == null, true);

    // popBatch spanning wrap-around boundary (head=2, cap=4 -> 2 from tail + 1 from front)
    let batch: [3]i64 = [0; 3];
    const popped = rb.popBatch(batch[0..3]);
    expect_eq(popped, 3 as usize);
    expect_eq(batch[0], 30 as i64);
    expect_eq(batch[1], 40 as i64);
    expect_eq(batch[2], 50 as i64);
    expect_eq(rb.len(), 1 as usize);
    expect_eq(rb.peek().?, 60 as i64);

    rb.clear();
    expect_eq(rb.isEmpty(), true);
    expect_eq(rb.pop() == null, true);
}

fn testRingBufferOverwriteOldest(): ! {
    const alloc = std.heap.page_allocator;
    let rb = RingBuffer(i64).init(alloc, 3, OverflowPolicy.OverwriteOldest)?;
    defer rb.deinit();

    rb.push(1)?;
    rb.push(2)?;
    rb.push(3)?;
    expect_eq(rb.overwrittenCount(), 0 as u64);

    // Overwrite 1, then 2, then 3 -> logical sequence becomes [4, 5, 6]
    rb.push(4)?;
    rb.push(5)?;
    rb.push(6)?;
    expect_eq(rb.overwrittenCount(), 3 as u64);
    expect_eq(rb.len(), 3 as usize);

    expect_eq(rb.peekAt(0).?, 4 as i64);
    expect_eq(rb.peekAt(1).?, 5 as i64);
    expect_eq(rb.peekAt(2).?, 6 as i64);

    expect_eq(rb.pop().?, 4 as i64);
    expect_eq(rb.pop().?, 5 as i64);
    expect_eq(rb.pop().?, 6 as i64);
    expect_eq(rb.isEmpty(), true);
    expect_eq(rb.overwrittenCount(), 3 as u64);
}

fn testByteRingWrapAroundPeekAndReadUntilByte(): ! {
    const alloc = std.heap.page_allocator;
    let pipe = ByteRing.init(alloc, 10)?;
    defer pipe.deinit();

    expect_eq(pipe.writableLen(), 10 as usize);
    expect_eq(pipe.readableLen(), 0 as usize);

    // Write 7 bytes, read 5 bytes -> head=5, count=2 ("67")
    const w1 = pipe.write("1234567");
    expect_eq(w1, 7 as usize);
    let tmp: [5]u8 = [0; 5];
    const r1 = pipe.read(tmp[0..5]);
    expect_eq(r1, 5 as usize);
    expect_eq(std.mem.eql(u8, tmp[0..5], "12345"), true);

    // Write 8 more bytes -> wraps around end of buffer (only 8 writable slots)
    const w2 = pipe.write("89ABCDEF");
    expect_eq(w2, 8 as usize);
    expect_eq(pipe.isFull(), true);

    // Peek 6 bytes without consuming
    let pbuf: [6]u8 = [0; 6];
    const pn = pipe.peek(pbuf[0..6]);
    expect_eq(pn, 6 as usize);
    expect_eq(std.mem.eql(u8, pbuf[0..6], "6789AB"), true);
    expect_eq(pipe.readableLen(), 10 as usize);

    // Skip 2 bytes ("67"), then read remaining 8 bytes across wrap-around
    expect_eq(pipe.skip(2), 2 as usize);
    let rbuf: [8]u8 = [0; 8];
    const rn = pipe.read(rbuf[0..8]);
    expect_eq(rn, 8 as usize);
    expect_eq(std.mem.eql(u8, rbuf[0..8], "89ABCDEF"), true);
    expect_eq(pipe.isEmpty(), true);

    // Incremental line framing via readUntilByte('\n') across wrap-around
    * = pipe.write("hello");
    const no_line = pipe.readUntilByte(alloc, '\n')?;
    expect_eq(no_line == null, true);

    * = pipe.write("\nwor");
    const line1 = (pipe.readUntilByte(alloc, '\n')?).?;
    defer alloc.free(line1);
    expect_eq(std.mem.eql(u8, line1, "hello\n"), true);

    * = pipe.write("ld\n");
    const line2 = (pipe.readUntilByte(alloc, '\n')?).?;
    defer alloc.free(line2);
    expect_eq(std.mem.eql(u8, line2, "world\n"), true);
    expect_eq(pipe.isEmpty(), true);
}

fn testMicroBatcherCountAndTimeoutTriggers(): ! {
    const alloc = std.heap.page_allocator;
    let mb = MicroBatcher(i32).init(alloc, 3, 100)?;
    defer mb.deinit();

    // Empty batcher never flushes
    expect_eq(mb.isEmpty(), true);
    expect_eq(mb.shouldFlush(99999), false);

    // 1. Count-based trigger
    expect_eq(mb.add(10, 1000)?, false);
    expect_eq(mb.shouldFlush(1050), false);
    expect_eq(mb.add(20, 1020)?, false);
    expect_eq(mb.add(30, 1030)?, true);
    expect_eq(mb.shouldFlush(1030), true);

    const b1 = mb.drain(alloc)?;
    defer alloc.free(b1);
    expect_eq(b1.len, 3 as usize);
    expect_eq(b1[0], 10 as i32);
    expect_eq(b1[1], 20 as i32);
    expect_eq(b1[2], 30 as i32);
    expect_eq(mb.isEmpty(), true);

    // 2. Time-window trigger after drain reset
    expect_eq(mb.add(42, 2000)?, false);
    expect_eq(mb.shouldFlush(2099), false);
    expect_eq(mb.shouldFlush(2100), true);

    const b2 = mb.drain(alloc)?;
    defer alloc.free(b2);
    expect_eq(b2.len, 1 as usize);
    expect_eq(b2[0], 42 as i32);
    expect_eq(mb.isEmpty(), true);
    expect_eq(mb.shouldFlush(5000), false);
}

fn testCapRingbufDynamicLazyMount(): ! {
    const alloc = std.heap.page_allocator;

    let rb = cap.ringbuf.RingBuffer(i32).init(alloc, 2, cap.ringbuf.OverflowPolicy.OverwriteOldest)?;
    defer rb.deinit();
    rb.push(100)?;
    rb.push(200)?;
    rb.push(300)?;
    expect_eq(rb.overwrittenCount(), 1 as u64);
    expect_eq(rb.pop().?, 200 as i32);

    let pipe = cap.ringbuf.ByteRing.init(alloc, 8)?;
    defer pipe.deinit();
    expect_eq(pipe.write("ok\n"), 3 as usize);
    const frame = (pipe.readUntilByte(alloc, '\n')?).?;
    defer alloc.free(frame);
    expect_eq(std.mem.eql(u8, frame, "ok\n"), true);

    let mb = cap.ringbuf.MicroBatcher(i32).init(alloc, 2, 50)?;
    defer mb.deinit();
    expect_eq(mb.add(7, 10)?, false);
    expect_eq(mb.add(8, 20)?, true);
    const out = mb.drain(alloc)?;
    defer alloc.free(out);
    expect_eq(out.len, 2 as usize);
}

fn main() {
    test("1. RingBuffer<i64> Reject Policy, BufferFull, Wrap-Around peekAt & popBatch", () => {
        testRingBufferRejectWrapAroundAndBatch()?;
    });
    test("2. RingBuffer<i64> OverwriteOldest Policy & overwrittenCount Accuracy", () => {
        testRingBufferOverwriteOldest()?;
    });
    test("3. ByteRing Wrap-Around Write/Read, Non-Consuming Peek & readUntilByte Line Framing", () => {
        testByteRingWrapAroundPeekAndReadUntilByte()?;
    });
    test("4. MicroBatcher<i32> Count Threshold vs Time Window Flush & Drain Reset", () => {
        testMicroBatcherCountAndTimeoutTriggers()?;
    });
    test("5. import cap from \"cap\"; cap.ringbuf Dynamic Lazy Mounting", () => {
        testCapRingbufDynamicLazyMount()?;
    });
}
