// Bench 2 (CP): High-frequency compute (fib(38)) + Zero-GC 10M u32 array hash reduction
import { HwTimer } from "cap:bench";
import std from "std";

const ARRAY_LEN: usize = 1_000_000;
const ROUNDS: usize = 10;

fn fib(n: i64): i64 {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

fn runComputeBench() {
    // Sub-bench 2A: Recursive fib(38) (prevent compile-time constant folding)
    let n_in: i64 = 38;
    std.mem.doNotOptimizeAway(&n_in);
    const t0 = HwTimer.nowNs();
    let fib_res = fib(n_in);
    std.mem.doNotOptimizeAway(&fib_res);
    const t1 = HwTimer.nowNs();

    // Sub-bench 2B: Allocate 1M u32 buffer + 10M element FNV-1a reduction (zero GC)
    const alloc = std.heap.page_allocator;
    let buf = alloc.alloc(u32, ARRAY_LEN) catch return;
    defer alloc.free(buf);

    const t2 = HwTimer.nowNs();
    let i: usize = 0;
    while (i < ARRAY_LEN) {
        buf[i] = @as(u32, @truncate(i)) *% 2654435761;
        i += 1;
    }

    let acc: u32 = 2166136261;
    let r: usize = 0;
    while (r < ROUNDS) {
        let j: usize = 0;
        while (j < ARRAY_LEN) {
            acc = (acc ^ buf[j]) *% 16777619;
            j += 1;
        }
        r += 1;
    }
    std.mem.doNotOptimizeAway(&acc);
    const t3 = HwTimer.nowNs();

    const fib_ms: f64 = @as(f64, @floatFromInt(t1 - t0)) / 1_000_000.0;
    const reduce_ms: f64 = @as(f64, @floatFromInt(t3 - t2)) / 1_000_000.0;
    const total_ms: f64 = fib_ms + reduce_ms;

    std.debug.print("{{\"fib_ms\":{d:.3},\"reduce_ms\":{d:.3},\"total_ms\":{d:.3},\"fib_res\":{d},\"reduce_hash\":{d}}}\n", .{
        fib_ms,
        reduce_ms,
        total_ms,
        fib_res,
        acc,
    });
}

fn main() {
    runComputeBench();
}
