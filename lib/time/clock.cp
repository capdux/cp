// Source: cp/lib/time/clock.cp (Monotonic Clock, Wall Clock & Duration Primitives)
// lib/time/clock.cp
// Zero-dependency Linux monotonic/wall clock primitives and fixed Duration arithmetic

pub enum TimeUnit: u8 {
    Nanoseconds = 0,
    Microseconds = 1,
    Milliseconds = 2,
    Seconds = 3,
    Minutes = 4,
    Hours = 5,
    Days = 6,
}

pub struct Duration {
    ns: i64 = 0;

    static fn fromNanos(ns: i64): Duration {
        return Duration { ns: ns };
    }

    static fn fromMicros(us: i64): Duration {
        return Duration { ns: us * 1000 };
    }

    static fn fromMillis(ms: i64): Duration {
        return Duration { ns: ms * 1000000 };
    }

    static fn fromSeconds(sec: i64): Duration {
        return Duration { ns: sec * 1000000000 };
    }

    static fn fromMinutes(min: i64): Duration {
        return Duration { ns: min * 60000000000 };
    }

    static fn fromHours(hrs: i64): Duration {
        return Duration { ns: hrs * 3600000000000 };
    }

    static fn fromDays(days: i64): Duration {
        return Duration { ns: days * 86400000000000 };
    }

    fn toNanos(): i64 {
        return _.ns;
    }

    fn toMicros(): i64 {
        return @divTrunc(_.ns, 1000);
    }

    fn toMillis(): i64 {
        return @divTrunc(_.ns, 1000000);
    }

    fn toSeconds(): i64 {
        return @divTrunc(_.ns, 1000000000);
    }

    fn add(other: Duration): Duration {
        return Duration { ns: _.ns + other.ns };
    }

    fn sub(other: Duration): Duration {
        return Duration { ns: _.ns - other.ns };
    }

    fn isZero(): bool {
        return _.ns == 0;
    }

    fn isNegative(): bool {
        return _.ns < 0;
    }

    fn abs(): Duration {
        return Duration { ns: if (_.ns < 0) (0 - _.ns) else _.ns };
    }
}

pub static struct WallClock {
    fn nowUnixNs(): i64 {
        let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
        zig {
            _ = std.os.linux.clock_gettime(.REALTIME, &ts);
        }
        return (ts.sec as i64) * 1000000000 + (ts.nsec as i64);
    }

    fn nowUnixMs(): i64 {
        let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
        zig {
            _ = std.os.linux.clock_gettime(.REALTIME, &ts);
        }
        return (ts.sec as i64) * 1000 + @divTrunc(ts.nsec as i64, 1000000);
    }

    fn nowUnixSec(): i64 {
        let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
        zig {
            _ = std.os.linux.clock_gettime(.REALTIME, &ts);
        }
        return ts.sec as i64;
    }

    fn monotonicNs(): i64 {
        let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
        zig {
            _ = std.os.linux.clock_gettime(.MONOTONIC, &ts);
        }
        return (ts.sec as i64) * 1000000000 + (ts.nsec as i64);
    }

    fn monotonicMs(): i64 {
        return @divTrunc(WallClock.monotonicNs(), 1000000);
    }
}
