// Source: /data/dev/bun/src/safety/safety.cp
import std from "std";

/// Assert condition is true; panic with message otherwise.
pub fn assert(condition: bool, msg: []const u8 = "assertion failure") {
    if (!condition) {
        @panic(msg);
    }
}

pub fn safety_assert(condition: bool, msg: []const u8 = "assertion failure") {
    assert(condition, msg);
}

/// Unconditional panic helper for unrecoverable safety states.
pub fn safety_panic(msg: []const u8 = "fatal safety panic"): noreturn {
    @panic(msg);
}

/// Marks unreachable code paths.
pub fn unreachable_code(msg: []const u8 = "reached unreachable code"): noreturn {
    @panic(msg);
}
