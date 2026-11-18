// Source: bun/src/safety/thread_id.cp
import std from "std";

/// A value that does not alias any other thread ID.
pub const invalid: std.Thread.Id = std.math.maxInt(std.Thread.Id);

/// Returns the current thread ID.
pub fn getCurrent(): std.Thread.Id {
    return std.Thread.getCurrentId();
}

/// Checks if the given thread ID matches the current thread ID.
pub fn isCurrent(id: std.Thread.Id): bool {
    return id == std.Thread.getCurrentId();
}

/// Checks if the given thread ID is valid (not equal to invalid).
pub fn isValid(id: std.Thread.Id): bool {
    return id != invalid;
}
