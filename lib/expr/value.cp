// Source: cp/lib/expr/value.cp (Sandboxed Expression Value & Environment)
// Core value AST, error taxonomy, structural equality, truthiness, and
// zero-side-effect read-only variable binding environment for `cap:expr`.

const std = @import("std");

/// Error set returned by expression parsing, validation, and sandboxed evaluation.
pub err ExprError {
    SyntaxError,
    TypeMismatch,
    DivisionByZero,
    UnknownIdentifier,
    UnknownFunction,
    InvalidArity,
    MaxDepthExceeded,
    StepLimitExceeded,
    OutOfMemory,
}

fn intEqualsFloat(i: i64, f: f64): bool {
    if (std.math.isNan(f) || std.math.isInf(f)) return false;
    if (@trunc(f) != f) return false;
    // -2^63 and 2^63 are exact in IEEE-754 f64; valid i64 range is [-2^63, 2^63 - 1].
    if (f < -9223372036854775808.0 || f >= 9223372036854775808.0) return false;
    return i == @as(i64, @intFromFloat(f));
}

/// Tagged dynamic value produced and consumed by the sandboxed expression engine.
pub enum ExprValue {
    Null,
    Bool: bool,
    Int: i64,
    Float: f64,
    Str: string,
    List: []ExprValue,

    static fn makeNull(): ExprValue {
        return .Null;
    }

    static fn makeBool(v: bool): ExprValue {
        return ExprValue.Bool(v);
    }

    static fn makeInt(v: i64): ExprValue {
        return ExprValue.Int(v);
    }

    static fn makeFloat(v: f64): ExprValue {
        return ExprValue.Float(v);
    }

    static fn makeStr(v: string): ExprValue {
        return ExprValue.Str(v);
    }

    static fn makeList(items: []ExprValue): ExprValue {
        return ExprValue.List(items);
    }

    fn isNull(): bool {
        return switch (_) {
            .Null => true,
            else => false,
        };
    }

    fn isBool(): bool {
        return switch (_) {
            .Bool => true,
            else => false,
        };
    }

    fn isInt(): bool {
        return switch (_) {
            .Int => true,
            else => false,
        };
    }

    fn isFloat(): bool {
        return switch (_) {
            .Float => true,
            else => false,
        };
    }

    fn isNumber(): bool {
        return switch (_) {
            .Int, .Float => true,
            else => false,
        };
    }

    fn isStr(): bool {
        return switch (_) {
            .Str => true,
            else => false,
        };
    }

    fn isList(): bool {
        return switch (_) {
            .List => true,
            else => false,
        };
    }

    fn asBool(): bool? {
        return switch (_) {
            .Bool b => b,
            else => null,
        };
    }

    fn asInt(): i64? {
        return switch (_) {
            .Int n => n,
            else => null,
        };
    }

    fn asFloat(): f64? {
        return switch (_) {
            .Float f => f,
            else => null,
        };
    }

    fn asNumber(): f64? {
        return switch (_) {
            .Int n => @as(f64, @floatFromInt(n)),
            .Float f => f,
            else => null,
        };
    }

    fn asStr(): string? {
        return switch (_) {
            .Str s => s,
            else => null,
        };
    }

    fn asList(): ([]ExprValue)? {
        return switch (_) {
            .List items => items,
            else => null,
        };
    }

    /// Evaluates truthiness across all value variants:
    /// - `Null` -> `false`
    /// - `Bool(b)` -> `b`
    /// - `Int(n)` -> `n != 0`
    /// - `Float(f)` -> `f != 0.0 && !isNan(f)`
    /// - `Str(s)` -> `s.len > 0`
    /// - `List(items)` -> `items.len > 0`
    fn isTruthy(): bool {
        return switch (_) {
            .Null => false,
            .Bool b => b,
            .Int n => n != 0,
            .Float f => f != 0.0 && !std.math.isNan(f),
            .Str s => s.len > 0,
            .List items => items.len > 0,
        };
    }

    /// Deep value equality (`Int` and `Float` compare numerically when exact; `List` compares recursively).
    fn eql(other: ExprValue): bool {
        return switch (_) {
            .Null => switch (other) {
                .Null => true,
                else => false,
            },
            .Bool a => switch (other) {
                .Bool b => a == b,
                else => false,
            },
            .Int a => switch (other) {
                .Int b => a == b,
                .Float b => intEqualsFloat(a, b),
                else => false,
            },
            .Float a => switch (other) {
                .Float b => a == b,
                .Int b => intEqualsFloat(b, a),
                else => false,
            },
            .Str a => switch (other) {
                .Str b => std.mem.eql(u8, a, b),
                else => false,
            },
            .List a => switch (other) {
                .List b => {
                    if (a.len != b.len) return false;
                    let i: usize = 0;
                    while (i < a.len) : (i += 1) {
                        if (!a[i].eql(b[i])) return false;
                    }
                    return true;
                },
                else => false,
            },
        };
    }
}

/// Zero-side-effect read-only evaluation environment supporting top-level keys
/// and dotted-path keys (e.g., `"user.role"`, `"request.tokens"`, `"env.region"`).
pub struct ExprEnv {
    allocator: std.mem.Allocator,
    bindings: std.StringHashMapUnmanaged(ExprValue) = .empty,

    /// Creates an empty variable binding environment backed by `alloc`.
    pub static fn init(alloc: std.mem.Allocator): ExprEnv {
        return ExprEnv{
            allocator: alloc,
            bindings: .empty,
        };
    }

    /// Releases all owned binding keys and hash-map storage.
    pub fn deinit(*_) {
        let it = _.bindings.keyIterator();
        while (k_ptr := it.next()) {
            unsafe {
                _.allocator.free(k_ptr.*);
            }
        }
        _.bindings.deinit(_.allocator);
    }

    /// Binds or updates `key` (top-level or dotted path) to `value`.
    pub fn set(*_, key: string, value: ExprValue): ! {
        if (existing := _.bindings.getPtr(key)) {
            unsafe {
                existing.* = value;
            }
            return;
        }
        const owned_key = _.allocator.dupe(u8, key) catch return ExprError.OutOfMemory;
        errdefer _.allocator.free(owned_key);
        _.bindings.put(_.allocator, owned_key, value) catch return ExprError.OutOfMemory;
    }

    /// Convenience setter for `ExprValue.Null`.
    pub fn setNull(*_, key: string): ! {
        return _.set(key, .Null);
    }

    /// Convenience setter for `ExprValue.Bool`.
    pub fn setBool(*_, key: string, v: bool): ! {
        return _.set(key, ExprValue.Bool(v));
    }

    /// Convenience setter for `ExprValue.Int`.
    pub fn setInt(*_, key: string, v: i64): ! {
        return _.set(key, ExprValue.Int(v));
    }

    /// Convenience setter for `ExprValue.Float`.
    pub fn setFloat(*_, key: string, v: f64): ! {
        return _.set(key, ExprValue.Float(v));
    }

    /// Convenience setter for `ExprValue.Str`.
    pub fn setStr(*_, key: string, v: string): ! {
        return _.set(key, ExprValue.Str(v));
    }

    /// Convenience setter for `ExprValue.List`.
    pub fn setList(*_, key: string, items: []ExprValue): ! {
        return _.set(key, ExprValue.List(items));
    }

    /// Looks up a variable or dotted-path key in read-only mode.
    pub fn get(key: string): ExprValue? {
        return _.bindings.get(key);
    }

    /// Returns `true` if `key` is bound in the environment.
    pub fn has(key: string): bool {
        return _.bindings.contains(key);
    }

    /// Returns the number of bound keys.
    pub fn len(): usize {
        return _.bindings.count();
    }

    /// Removes all bound keys while keeping the environment usable.
    pub fn clear(*_) {
        let it = _.bindings.keyIterator();
        while (k_ptr := it.next()) {
            unsafe {
                _.allocator.free(k_ptr.*);
            }
        }
        _.bindings.clearRetainingCapacity();
    }
}
