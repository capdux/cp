// Source: /data/dev/bun/src/bun_alloc/ & /data/dev/bun/src/ptr/raw_ref_count.cp
// lib/mem/rc.cp
// Deterministic Reference Counting & Weak Control Block (absorbed from bun/src/ptr/raw_ref_count.cp, ref_count.cp, weak_ptr.cp)

pub enum DecrementResult {
    KeepAlive,
    ShouldDestroy,
}

// Single-threaded deterministic reference counter
pub struct RefCounter {
    raw_value: u32 = 1;

    pub static fn init(initial_count: u32): RefCounter {
        return RefCounter{
            raw_value: initial_count,
        };
    }

    pub fn increment(*_): u32 {
        _.raw_value += 1;
        return _.raw_value;
    }

    pub fn decrement(*_): DecrementResult {
        _.raw_value -= 1;
        if (_.raw_value == 0) {
            return .ShouldDestroy;
        }
        return .KeepAlive;
    }

    pub fn count(): u32 {
        return _.raw_value;
    }

    pub fn isUnique(*_): bool {
        return _.raw_value == 1;
    }
}

// Thread-safe atomic reference counter
pub struct AtomicRefCounter {
    raw_value: u32 = 1;

    pub static fn init(initial_count: u32): AtomicRefCounter {
        return AtomicRefCounter{
            raw_value: initial_count,
        };
    }

    pub fn increment(*_): u32 {
        let prev: u32 = 0;
        zig {
            prev = @atomicRmw(u32, &self.raw_value, .Add, 1, .monotonic);
        }
        return prev + 1;
    }

    pub fn decrement(*_): DecrementResult {
        let prev: u32 = 0;
        zig {
            prev = @atomicRmw(u32, &self.raw_value, .Sub, 1, .acq_rel);
        }
        if (prev == 1) {
            return .ShouldDestroy;
        }
        return .KeepAlive;
    }

    pub fn count(): u32 {
        let cur: u32 = 0;
        zig {
            cur = @atomicLoad(u32, &self.raw_value, .monotonic);
        }
        return cur;
    }
}

// Weak reference control block (Absorbed from bun/src/ptr/weak_ptr.cp)
pub struct WeakControlBlock {
    strong_count: u32 = 1;
    weak_count: u32 = 0;
    finalized: bool = false;

    pub static fn init(): WeakControlBlock {
        return WeakControlBlock{
            strong_count: 1,
            weak_count: 0,
            finalized: false,
        };
    }

    pub fn incStrong(*_): bool {
        if (_.finalized || _.strong_count == 0) {
            return false;
        }
        _.strong_count += 1;
        return true;
    }

    pub fn decStrong(*_): DecrementResult {
        _.strong_count -= 1;
        if (_.strong_count == 0) {
            _.finalized = true;
            return .ShouldDestroy;
        }
        return .KeepAlive;
    }

    pub fn incWeak(*_): u32 {
        _.weak_count += 1;
        return _.weak_count;
    }

    pub fn decWeak(*_): bool {
        _.weak_count -= 1;
        return _.finalized && _.weak_count == 0;
    }

    pub fn isAlive(): bool {
        return !_.finalized && _.strong_count > 0;
    }

    pub fn canDeallocate(*_): bool {
        return _.finalized && _.strong_count == 0 && _.weak_count == 0;
    }

    pub fn strongCount(): u32 {
        return _.strong_count;
    }

    pub fn weakCount(): u32 {
        return _.weak_count;
    }
}

pub type Rc = RefCounter;
