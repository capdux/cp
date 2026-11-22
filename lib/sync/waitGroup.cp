// Source: Absorbed from /data/dev/bun/src/threading/WaitGroup.cp | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// Absorbed from Bun: bun/src/threading/wait_group.cp
// Upstream Sync: Refer to bun/src/threading/wait_group.zig for future upstream updates.
// ============================================================================
// WaitGroup synchronization primitive in CP
// Ported from Bun.js core threading architecture
import mutexMod from "./mutex";
const Mutex = mutexMod.Mutex;
import { Condition } from "./condition";

pub struct WaitGroup {
    raw_count: std.atomic.Value(usize) = .{ .raw = 0 },
    mutex: Mutex = .{},
    cond: Condition = .{},

    pub static fn init(): WaitGroup {
        return .{};
    }

    pub static fn initWithCount(count: usize): WaitGroup {
        return .{ .raw_count = .{ .raw = count } };
    }

    pub fn addUnsynchronized(*_, n: usize) {
        _.raw_count.raw = _.raw_count.raw + n;
    }

    pub fn add(*_, n: usize) {
        * = _.raw_count.fetchAdd(n, .monotonic);
    }

    pub fn addOne(*_) {
        _.add(1);
    }

    pub fn finish(*_) {
        if (_.raw_count.fetchSub(1, .acq_rel) > 1) {
            return;
        }

        _.mutex.lock();
        _.mutex.unlock();
        _.cond.signal();
    }

    pub fn done(*_) {
        _.finish();
    }

    pub fn deinit(*_) {
    }

    pub fn wait(*_) {
        _.mutex.lock();
        defer _.mutex.unlock();

        while (_.raw_count.load(.acquire) > 0) {
            _.cond.wait(&_.mutex);
        }
    }

    ~init() {
        _.cond.~init();
    }
}
