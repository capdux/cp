// Source: cp/lib/ulid/generator.cp (Monotonic ULID Generator & CSPRNG Entropy)
// Ensures strict lexicographical monotonicity within the same millisecond.

const std = @import("std");
import {
    ULID_ENTROPY_LEN,
    ULID_MAX_TIMESTAMP,
    UlidError,
} from "./types.cp";
import { Ulid } from "./ulid.cp";

fn fillCsprng(buf: []u8) {
    * = std.os.linux.getrandom(buf.ptr, buf.len, 0);
}

fn currentUnixMs(): u64 {
    const ms = std.time.milliTimestamp();
    if (ms < 0) {
        return 0;
    }
    return ms as u64;
}

fn increment80BitBe(ent: *[10]u8): bool {
    let idx: usize = ULID_ENTROPY_LEN;
    while (idx > 0) {
        idx -= 1;
        if (ent[idx] != 0xFF) {
            ent[idx] += 1;
            return true;
        }
        ent[idx] = 0;
    }
    return false;
}

pub struct MonotonicUlidGenerator {
    last_ms: u64 = 0,
    last_entropy: [10]u8 = [0; 10],
    initialized: bool = false,
    has_seed: bool = false,
    seed_entropy: [10]u8 = [0; 10],

    /// Initializes a monotonic generator, optionally seeded with deterministic initial 80-bit entropy.
    pub static fn init(seed_bytes_opt: ([10]u8)?): MonotonicUlidGenerator {
        if (seed := seed_bytes_opt) {
            return MonotonicUlidGenerator{
                last_ms: 0,
                last_entropy: [0; 10],
                initialized: false,
                has_seed: true,
                seed_entropy: seed,
            };
        }
        return MonotonicUlidGenerator{
            last_ms: 0,
            last_entropy: [0; 10],
            initialized: false,
            has_seed: false,
            seed_entropy: [0; 10],
        };
    }

    /// Generates the next strictly monotonic ULID at explicit timestamp `now_ms`.
    pub fn generateAt(*_, now_ms: u64): Ulid ! UlidError {
        if (now_ms > ULID_MAX_TIMESTAMP) {
            return UlidError.TimestampOverflow;
        }

        if (!_.initialized || now_ms > _.last_ms) {
            if (!_.initialized && _.has_seed) {
                _.last_entropy = _.seed_entropy;
            } else {
                fillCsprng(_.last_entropy[0..ULID_ENTROPY_LEN]);
            }
            _.last_ms = now_ms;
            _.initialized = true;
        } else {
            if (!increment80BitBe(&_.last_entropy)) {
                return UlidError.MonotonicOverflow;
            }
        }

        return Ulid.fromParts(_.last_ms, _.last_entropy);
    }

    /// Generates the next strictly monotonic ULID using the current wall-clock millisecond time.
    pub fn generate(*_): Ulid ! UlidError {
        const now_ms = currentUnixMs();
        return _.generateAt(now_ms);
    }
}
