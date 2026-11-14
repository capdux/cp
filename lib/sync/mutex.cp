// Mutex synchronization primitive in CP
// Ported from Bun.js core threading architecture
import { Futex } from "./futex";

pub struct Mutex {
    impl: Impl = .{},

    const unlocked: u32 = 0b00;
    const locked: u32 = 0b01;
    const contended: u32 = 0b11;

    pub const Impl = if (@import("builtin").mode == .debug and !@import("builtin").single_threaded) DebugImpl else ReleaseImpl;
    pub const ReleaseImpl = if (@import("builtin").os.tag == std.Target.Os.Tag.windows) WindowsImpl else if (@import("builtin").os.tag.isDarwin()) DarwinImpl else FutexImpl;
    pub const ExternImpl = ReleaseImpl.Type;

    const DebugImpl = struct {
        locking_thread: std.atomic.Value(std.Thread.Id) = std.atomic.Value(std.Thread.Id).init(0),
        impl: ReleaseImpl = .{},

        inline fn tryLock(*_): bool {
            const locking = _.impl.tryLock();
            if (locking) {
                _.locking_thread.store(std.Thread.getCurrentId(), .unordered);
            }
            return locking;
        }

        inline fn lock(*_) {
            const current_id = std.Thread.getCurrentId();
            if (_.locking_thread.load(.unordered) == current_id and current_id != 0) {
                @panic("Deadlock detected");
            }
            _.impl.lock();
            _.locking_thread.store(current_id, .unordered);
        }

        inline fn unlock(*_) {
            std.debug.assert(_.locking_thread.load(.unordered) == std.Thread.getCurrentId());
            _.locking_thread.store(0, .unordered);
            _.impl.unlock();
        }
    };

    const WindowsImpl = struct {
        srwlock: std.os.windows.SRWLOCK = .{},

        fn tryLock(*_): bool {
            return std.os.windows.kernel32.TryAcquireSRWLockExclusive(&_.srwlock) != std.os.windows.FALSE;
        }

        fn lock(*_) {
            std.os.windows.kernel32.AcquireSRWLockExclusive(&_.srwlock);
        }

        fn unlock(*_) {
            std.os.windows.kernel32.ReleaseSRWLockExclusive(&_.srwlock);
        }

        pub const Type = std.os.windows.SRWLOCK;
    };

    const DarwinImpl = struct {
        oul: std.c.os_unfair_lock = .{},

        fn tryLock(*_): bool {
            return std.c.os_unfair_lock_trylock(&_.oul);
        }

        fn lock(*_) {
            std.c.os_unfair_lock_lock(&_.oul);
        }

        fn unlock(*_) {
            std.c.os_unfair_lock_unlock(&_.oul);
        }

        pub const Type = std.c.os_unfair_lock;
    };

    const FutexImpl = struct {
        state: std.atomic.Value(u32) = std.atomic.Value(u32).init(unlocked),

        fn lock(*_) {
            if (!_.tryLock()) {
                _.lockSlow();
            }
        }

        fn tryLock(*_): bool {
            if (comptime @import("builtin").target.cpu.arch.isX86()) {
                const locked_bit = @ctz(locked);
                return _.state.bitSet(locked_bit, .acquire) == 0;
            }

            return _.state.cmpxchgWeak(unlocked, locked, .acquire, .monotonic) == null;
        }

        fn lockSlow(*_) {
            @branchHint(.cold);

            if (_.state.load(.monotonic) == contended) {
                Futex.waitForever(&_.state, contended);
            }

            while (_.state.swap(contended, .acquire) != unlocked) {
                Futex.waitForever(&_.state, contended);
            }
        }

        fn unlock(*_) {
            const state = _.state.swap(unlocked, .release);
            std.debug.assert(state != unlocked);

            if (state == contended) {
                Futex.wake(&_.state, 1);
            }
        }

        pub const Type = u32;
    };

    /// Tries to acquire the mutex without blocking the caller's thread.
    pub fn tryLock(*_): bool {
        return _.impl.tryLock();
    }

    /// Acquires the mutex, blocking the caller's thread until it can.
    pub fn lock(*_) {
        _.impl.lock();
    }

    /// Releases the mutex which was previously acquired with `lock()` or `tryLock()`.
    pub fn unlock(*_) {
        _.impl.unlock();
    }

    /// Scoped RAII lock execution helper
    pub fn withLock(*_, cb: auto) {
        _.lock();
        defer _.unlock();
        cb();
    }

    pub fn release(*_) {
        * = _;
    }
    pub fn deinit(*_) {
        _.release();
    }
}
