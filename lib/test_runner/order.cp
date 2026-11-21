// Source: Absorbed from /data/dev/bun/src/test_runner/Order.cp | Synced: 2026-11-21T09:33:20Z
// ============================================================================
// lib/test_runner/order.cp — Deterministic Test Ordering & Seeded PRNG
// ============================================================================

const std = @import("std");

pub enum TestOrderMode: u8 {
    Declaration = 0,
    Random = 1,
    Reverse = 2,

    fn label(): []const u8 {
        return switch (_) {
            .Declaration => "declaration",
            .Random => "random",
            .Reverse => "reverse",
        };
    }
}

/// Deterministic 64-bit PRNG backed by SplitMix64 seeding and Xoshiro256++ generation.
pub struct Prng64 {
    s: [4]u64 = [0, 0, 0, 0],

    pub static fn splitMix64(state: *u64): u64 {
        let out: u64 = 0;
        zig {
            state.* +%= 0x9e3779b97f4a7c15;
            var z: u64 = state.*;
            z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
            z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
            out = z ^ (z >> 31);
        }
        return out;
    }

    static fn rotl64(x: u64, k: u6): u64 {
        return (x << k) | (x >> (@as(u6, 0) -% k));
    }

    pub static fn init(seed: u64): Prng64 {
        let fallback: u64 = 0;
        zig {
            fallback = 0x9e3779b97f4a7c15;
        }
        let sm_state: u64 = if (seed == 0) fallback else seed;
        let st: [4]u64 = [
            Prng64.splitMix64(*sm_state),
            Prng64.splitMix64(*sm_state),
            Prng64.splitMix64(*sm_state),
            Prng64.splitMix64(*sm_state),
        ];
        return Prng64{ s: st };
    }

    /// Derives a reproducible per-file/per-suite 64-bit seed from `(base_seed, salt)`.
    pub static fn deriveSeed(base_seed: u64, salt: []const u8): u64 {
        let h: u64 = 0;
        zig {
            var hasher = std.hash.Wyhash.init(base_seed);
            hasher.update(salt);
            h = hasher.final();
        }
        return Prng64.splitMix64(*h);
    }

    pub fn nextU64(*_): u64 {
        const result = Prng64.rotl64(_.s[0] +% _.s[3], 23) +% _.s[0];
        const t = _.s[1] << 17;

        _.s[2] ^= _.s[0];
        _.s[3] ^= _.s[1];
        _.s[1] ^= _.s[2];
        _.s[0] ^= _.s[3];

        _.s[2] ^= t;
        _.s[3] = Prng64.rotl64(_.s[3], 45);
        return result;
    }

    /// Returns a uniform integer in `[0, bound)` using unbiased rejection sampling.
    pub fn nextBounded(*_, bound: usize): usize {
        if (bound <= 1) return 0;
        const b64: u64 = bound as u64;
        const zero_u64: u64 = 0;
        const threshold: u64 = (zero_u64 -% b64) % b64;
        while (true) {
            const r = _.nextU64();
            if (r >= threshold) {
                return (r % b64) as usize;
            }
        }
    }

    pub fn nextF64(*_): f64 {
        const top53: u64 = _.nextU64() >> 11;
        return (top53 as f64) / 9007199254740992.0;
    }
}

/// Deterministic test execution order planner (Declaration, Random Fisher-Yates, Reverse).
pub struct TestOrder {
    /// Performs an in-place deterministic Fisher-Yates shuffle on `indices` using `seed`.
    pub static fn shuffleIndices(indices: []usize, seed: u64) {
        if (indices.len <= 1) return;
        let rng = Prng64.init(seed);
        let i: usize = indices.len - 1;
        while (i > 0) : (i -= 1) {
            const j = rng.nextBounded(i + 1);
            const tmp = indices[i];
            indices[i] = indices[j];
            indices[j] = tmp;
        }
    }

    /// Reverses `indices` in-place.
    pub static fn reverseIndices(indices: []usize) {
        if (indices.len <= 1) return;
        let lo: usize = 0;
        let hi: usize = indices.len - 1;
        while (lo < hi) {
            const tmp = indices[lo];
            indices[lo] = indices[hi];
            indices[hi] = tmp;
            lo += 1;
            hi -= 1;
        }
    }

    /// Allocates an index permutation `[0..count)` ordered according to `mode` and `seed`.
    pub static fn orderIndices(
        allocator: std.mem.Allocator,
        count: usize,
        mode: TestOrderMode,
        seed: u64,
    ): ([]usize)! {
        let out = allocator.alloc(usize, count)?;
        let i: usize = 0;
        while (i < count) : (i += 1) {
            out[i] = i;
        }
        switch (mode) {
            .Declaration => {},
            .Random => TestOrder.shuffleIndices(out, seed),
            .Reverse => TestOrder.reverseIndices(out),
        }
        return out;
    }

    /// Formats a reproducible seed banner for test output.
    pub static fn formatSeedBanner(allocator: std.mem.Allocator, seed: u64): ([]u8)! {
        return std.fmt.allocPrint(allocator, "Randomized test order with --seed={d}", .{seed});
    }
}
