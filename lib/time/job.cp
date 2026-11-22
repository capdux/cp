// Source: cp/lib/queue/job.cp (Job Types, Priority Levels & Exponential Retry Policy)
// Core data structures, priority levels, lifecycle states, and overflow-safe
// exponential backoff retry policy for the cap:queue background job scheduler.

const std = @import("std");

/// Error set returned by `JobQueue` operations.
pub err QueueError {
    JobNotFound,
    InvalidState,
    DuplicateId,
    QueueFull,
    OutOfMemory,
}

/// Scheduling priority tiers for background jobs (higher numeric value = higher priority).
pub enum JobPriority: u8 {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3,

    /// Converts priority enum to its integer rank `[0..3]`.
    fn rank(): u8 {
        return _ as u8;
    }

    /// Parses an integer rank `[0..3]` into a `JobPriority`, returning `null` if out of range.
    static fn fromRank(val: u8): JobPriority? {
        return switch (val) {
            0 => .Low,
            1 => .Normal,
            2 => .High,
            3 => .Critical,
            else => null,
        };
    }
}

/// Lifecycle state of a scheduled job.
pub enum JobState: u8 {
    Pending = 0,
    Delayed = 1,
    Running = 2,
    Completed = 3,
    DeadLetter = 4,

    /// Returns `true` if the job is in a terminal state (`Completed` or `DeadLetter`).
    fn isTerminal(): bool {
        return _ == .Completed || _ == .DeadLetter;
    }
}

/// Exponential backoff retry policy with configurable attempt cap and delay ceiling.
pub struct RetryPolicy {
    pub max_attempts: u32 = 3;
    pub base_delay_ms: i64 = 1000;
    pub max_delay_ms: i64 = 60000;

    /// Returns the default retry policy (`max_attempts = 3`, `base = 1000ms`, `max = 60000ms`).
    pub static fn default(): RetryPolicy {
        return RetryPolicy{
            max_attempts: 3,
            base_delay_ms: 1000,
            max_delay_ms: 60000,
        };
    }

    /// Constructs a custom `RetryPolicy`.
    pub static fn init(max_attempts: u32, base_delay_ms: i64, max_delay_ms: i64): RetryPolicy {
        return RetryPolicy{
            max_attempts: max_attempts,
            base_delay_ms: base_delay_ms,
            max_delay_ms: max_delay_ms,
        };
    }

    /// Computes capped exponential backoff delay in milliseconds:
    /// `min(max_delay_ms, base_delay_ms * 2^attempt)`, overflow-safe for large `attempt`.
    pub fn nextDelayMs(attempt: u32): i64 {
        if (_.base_delay_ms <= 0 || _.max_delay_ms <= 0) {
            return 0;
        }
        if (_.base_delay_ms >= _.max_delay_ms) {
            return _.max_delay_ms;
        }
        let delay: i64 = _.base_delay_ms;
        let i: u32 = 0;
        while (i < attempt) : (i += 1) {
            if (delay >= _.max_delay_ms) {
                return _.max_delay_ms;
            }
            if (delay > @divTrunc(_.max_delay_ms, 2)) {
                return _.max_delay_ms;
            }
            delay *= 2;
        }
        if (delay > _.max_delay_ms) {
            return _.max_delay_ms;
        }
        return delay;
    }
}

/// Immutable-by-convention snapshot / record of a background job in `JobQueue`.
pub struct Job {
    pub id: u64 = 0;
    pub topic: string = "";
    pub payload: string = "";
    pub priority: JobPriority = .Normal;
    pub state: JobState = .Pending;
    pub attempts: u32 = 0;
    pub max_attempts: u32 = 3;
    pub run_at_ms: i64 = 0;
    pub created_at_ms: i64 = 0;
    pub lease_until_ms: i64 = 0;
    pub last_error: string = "";
}
