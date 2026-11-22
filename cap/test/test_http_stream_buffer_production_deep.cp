import std from "std";
import { test, expect_eq } from "cap:test";
import {
    DEFAULT_STREAM_SHRINK_THRESHOLD,
    StreamBufferError,
    StreamDrainCallback,
    StreamBuffer,
    ThreadSafeStreamBuffer,
} from "cap:http";

let g_drain_hits: usize = 0;
let g_last_ctx: usize = 0;
let g_reentrant_was_empty: bool = false;
let g_reentrant_wrote_ok: bool = false;

fn onBasicDrain(ctx: usize) {
    g_drain_hits += 1;
    g_last_ctx = ctx;
}

fn onReentrantDrain(ctx: usize) {
    g_drain_hits += 1;
    unsafe {
        const ts_ptr: *ThreadSafeStreamBuffer = @ptrFromInt(ctx);
        g_reentrant_was_empty = ts_ptr.isEmpty();
        ts_ptr.write("refilled") catch {
            g_reentrant_wrote_ok = false;
            return;
        };
        g_reentrant_wrote_ok = true;
    }
}

test("1. StreamBuffer write, slice, writeByte, wrote, consume, and safe boundary clamp", () => {
    const alloc = std.testing.allocator;
    let buf = StreamBuffer.init(alloc);
    defer buf.deinit();

    expect_eq(buf.isEmpty(), true);
    expect_eq(buf.isNotEmpty(), false);
    expect_eq(buf.size(), usize(0));
    expect_eq(buf.slice().len, usize(0));

    // 空写入不应改变状态
    buf.write("") catch unreachable;
    expect_eq(buf.isEmpty(), true);

    // 正常写入 + 单字节写入
    buf.write("HTTP/1.1 ") catch unreachable;
    buf.writeByte('2') catch unreachable;
    buf.write("00 OK") catch unreachable;

    expect_eq(buf.isNotEmpty(), true);
    expect_eq(buf.size(), usize(15));
    expect_eq(std.mem.eql(u8, buf.slice(), "HTTP/1.1 200 OK"), true);

    // 零拷贝部分消费
    buf.wrote(9);
    expect_eq(buf.size(), usize(6));
    expect_eq(std.mem.eql(u8, buf.slice(), "200 OK"), true);

    // 拷贝消费到栈缓冲
    let out: [4]u8 = [0; 4];
    const n1 = buf.consume(out[0..]);
    expect_eq(n1, usize(4));
    expect_eq(std.mem.eql(u8, out[0..4], "200 "), true);
    expect_eq(buf.size(), usize(2));

    // 超出剩余长度的 wrote() 应安全截断并归零，不发生下溢
    buf.wrote(999);
    expect_eq(buf.isEmpty(), true);
    expect_eq(buf.size(), usize(0));
    expect_eq(buf.consume(out[0..]), usize(0));
});

test("2. StreamBuffer auto-compact prevents cursor drift and maybeShrink reclaims oversized buffer", () => {
    const alloc = std.testing.allocator;
    let buf = StreamBuffer.init(alloc);
    defer buf.deinit();

    // 构造超过 4096 字节的大块载荷，验证容量增长与排空收缩
    let big: [6144]u8 = [0x41; 6144];
    buf.write(big[0..]) catch unreachable;
    expect_eq(buf.size(), usize(6144));
    expect_eq(buf.memoryCost() >= 6144, true);

    // 消费 3072 字节后再次写入，应触发内部 compact() 回收头部空洞
    buf.wrote(3072);
    expect_eq(buf.cursor, usize(3072));
    buf.write("tail") catch unreachable;
    expect_eq(buf.cursor, usize(0));
    expect_eq(buf.size(), usize(3076));
    expect_eq(std.mem.eql(u8, buf.slice()[3072..3076], "tail"), true);

    // 全部消费后触发 reset() -> maybeShrink() 收缩至不超过 DEFAULT_STREAM_SHRINK_THRESHOLD
    buf.wrote(3076);
    expect_eq(buf.isEmpty(), true);
    expect_eq(buf.memoryCost() <= DEFAULT_STREAM_SHRINK_THRESHOLD, true);
});

test("3. StreamBuffer bounded capacity protection against resource exhaustion", () => {
    const alloc = std.testing.allocator;
    let buf = StreamBuffer.initBounded(alloc, 16);
    defer buf.deinit();

    // 恰好写满 16 字节应成功
    buf.write("0123456789abcdef") catch unreachable;
    expect_eq(buf.size(), usize(16));

    // 超限写入应返回 BufferOverflow
    let overflowed = false;
    buf.write("!") catch |e| {
        if (e == StreamBufferError.BufferOverflow) {
            overflowed = true;
        }
    };
    expect_eq(overflowed, true);

    // ensureUnusedCapacity 超限同样防护
    let cap_overflowed = false;
    buf.ensureUnusedCapacity(1) catch |e| {
        if (e == StreamBufferError.BufferOverflow) {
            cap_overflowed = true;
        }
    };
    expect_eq(cap_overflowed, true);

    // 消费 6 字节释放空间后，写入 6 字节应恢复成功
    buf.wrote(6);
    expect_eq(buf.size(), usize(10));
    buf.write("ABCDEF") catch unreachable;
    expect_eq(buf.size(), usize(16));
    expect_eq(std.mem.eql(u8, buf.slice(), "6789abcdefABCDEF"), true);
});

test("4. ThreadSafeStreamBuffer acquire/release zero-copy lock section and read/write API", () => {
    const alloc = std.testing.allocator;
    let ts = ThreadSafeStreamBuffer.init(alloc);
    defer ts.deinit();

    expect_eq(ts.isEmpty(), true);
    expect_eq(ts.size(), usize(0));

    // Bun 原版风格：acquire() -> 批量零拷贝操作 -> release()
    {
        let inner = ts.acquire();
        inner.write("chunk-1|") catch unreachable;
        inner.write("chunk-2") catch unreachable;
        expect_eq(std.mem.eql(u8, inner.slice(), "chunk-1|chunk-2"), true);
        ts.release();
    }

    expect_eq(ts.size(), usize(15));
    expect_eq(ts.memoryCost() >= 15, true);

    let out: [32]u8 = [0; 32];
    const n = ts.read(out[0..]);
    expect_eq(n, usize(15));
    expect_eq(std.mem.eql(u8, out[0..15], "chunk-1|chunk-2"), true);
    expect_eq(ts.isEmpty(), true);
});

test("5. ThreadSafeStreamBuffer high-water backpressure detection and reset recovery", () => {
    const alloc = std.testing.allocator;
    let ts = ThreadSafeStreamBuffer.initBounded(alloc, 32, 10);
    defer ts.deinit();

    expect_eq(ts.isBackpressured(), false);

    ts.write("123456789") catch unreachable; // 9 < 10
    expect_eq(ts.isBackpressured(), false);

    ts.write("0") catch unreachable; // 10 >= 10 -> 触发背压
    expect_eq(ts.isBackpressured(), true);

    let small: [4]u8 = [0; 4];
    expect_eq(ts.read(small[0..]), usize(4)); // 剩余 6 < 10 -> 解除背压
    expect_eq(ts.isBackpressured(), false);

    ts.write("abcdefghij") catch unreachable; // 16 >= 10 -> 再次背压
    expect_eq(ts.isBackpressured(), true);

    ts.reset();
    expect_eq(ts.isEmpty(), true);
    expect_eq(ts.isBackpressured(), false);
});

test("6. ThreadSafeStreamBuffer drain callback lifecycle: set, skip non-empty, trigger empty, clear", () => {
    const alloc = std.testing.allocator;
    let ts = ThreadSafeStreamBuffer.init(alloc);
    defer ts.deinit();

    g_drain_hits = 0;
    g_last_ctx = 0;

    // 未注册回调时 reportDrain 返回 false
    expect_eq(ts.reportDrain(), false);
    expect_eq(ts.drainCount(), u64(0));

    ts.setDrainCallback(StreamDrainCallback.init(onBasicDrain, 0xCAFE));

    // 非空时不应触发回调
    ts.write("payload") catch unreachable;
    expect_eq(ts.reportDrain(), false);
    expect_eq(g_drain_hits, usize(0));
    expect_eq(ts.drainCount(), u64(0));

    // 消费完毕后触发排空回调
    let sink: [16]u8 = [0; 16];
    expect_eq(ts.read(sink[0..]), usize(7));
    expect_eq(ts.reportDrain(), true);
    expect_eq(g_drain_hits, usize(1));
    expect_eq(g_last_ctx, usize(0xCAFE));
    expect_eq(ts.drainCount(), u64(1));

    // 清除回调后不再触发
    ts.clearDrainCallback();
    expect_eq(ts.reportDrain(), false);
    expect_eq(g_drain_hits, usize(1));
    expect_eq(ts.drainCount(), u64(1));
});

test("7. ThreadSafeStreamBuffer reportDrain unlocks before invoking callback (reentrant deadlock-free)", () => {
    const alloc = std.testing.allocator;
    let ts = ThreadSafeStreamBuffer.init(alloc);
    defer ts.deinit();

    g_drain_hits = 0;
    g_reentrant_was_empty = false;
    g_reentrant_wrote_ok = false;

    const ptr_ctx: usize = unsafe { @intFromPtr(&ts) };
    ts.setDrainCallback(StreamDrainCallback.init(onReentrantDrain, ptr_ctx));

    // 回调内部重新调用 ts.isEmpty() 与 ts.write("refilled")，若未解锁将触发 Mutex 死锁检测
    expect_eq(ts.reportDrain(), true);
    expect_eq(g_drain_hits, usize(1));
    expect_eq(g_reentrant_was_empty, true);
    expect_eq(g_reentrant_wrote_ok, true);
    expect_eq(ts.size(), usize(8));

    let out: [16]u8 = [0; 16];
    const n = ts.read(out[0..]);
    expect_eq(n, usize(8));
    expect_eq(std.mem.eql(u8, out[0..8], "refilled"), true);
});
