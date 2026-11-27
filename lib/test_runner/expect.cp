// ============================================================================
// lib/test_runner/expect.cp — Bun/Jest-Compatible Chainable Expectation Matchers
// ============================================================================

const std = @import("std");
import { isCloseTo, includesSubstring } from "./expectMatch";
import { isNaNF64, isFiniteF64, isPositiveF64, isNegativeF64 } from "./expectNum";

/// Normalizes comptime-only Zig types (`comptime_int`, `comptime_float`, `null`)
/// into runtime-storable types so `expect(42)` and `expect(null)` can be returned.
pub fn NormalizeExpectType(comptime T: type): type {
    zig {
        return switch (T) {
            comptime_int => i64,
            comptime_float => f64,
            @TypeOf(null) => ?*const anyopaque,
            else => T,
        };
    }
}

/// Reports an assertion failure to stderr and increments the global test failure counter.
pub fn reportExpectFailure(label: []const u8, detail: []const u8) {
    zig {
        std.debug.print("\n    ❌ Assertion Failed ({s}): {s}\n", .{ label, detail });
        if (@hasDecl(__cp_rt, "__cap_test_failed")) {
            __cp_rt.__cap_test_failed += 1;
        }
    }
}

fn formatValueAlloc(allocator: std.mem.Allocator, val: auto): []const u8 {
    zig {
        const T = @TypeOf(val);
        if (T == []const u8 or T == []u8) {
            return std.fmt.allocPrint(allocator, "\"{s}\"", .{val}) catch "<str>";
        }
        const ti = @typeInfo(T);
        if (ti == .pointer and ti.pointer.size == .one) {
            const cti = @typeInfo(ti.pointer.child);
            if (cti == .array and cti.array.child == u8) {
                const sl: []const u8 = val;
                return std.fmt.allocPrint(allocator, "\"{s}\"", .{sl}) catch "<str>";
            }
        }
        return std.fmt.allocPrint(allocator, "{any}", .{val}) catch "<val>";
    }
}

fn failWithExpectedGot(label: []const u8, negated: bool, expected: auto, actual: auto) {
    zig {
        var stack_buf: [1024]u8 = undefined;
        var fba = std.heap.FixedBufferAllocator.init(&stack_buf);
        const alloc = fba.allocator();
        const exp_s = formatValueAlloc(alloc, expected);
        const act_s = formatValueAlloc(alloc, actual);
        if (negated) {
            std.debug.print("\n    ❌ Assertion Failed ({s}):\n    Expected: not {s}\n    Received: {s}\n", .{ label, exp_s, act_s });
        } else {
            std.debug.print("\n    ❌ Assertion Failed ({s}):\n    Expected: {s}\n    Received: {s}\n", .{ label, exp_s, act_s });
        }
        if (@hasDecl(__cp_rt, "__cap_test_failed")) {
            __cp_rt.__cap_test_failed += 1;
        }
    }
}

fn isStringLikeType(comptime T: type): bool {
    zig {
        if (T == []const u8 or T == []u8) return true;
        const ti = @typeInfo(T);
        if (ti == .pointer and ti.pointer.size == .one) {
            const cti = @typeInfo(ti.pointer.child);
            if (cti == .array and cti.array.child == u8) return true;
        }
        return false;
    }
}

fn asByteSlice(val: auto): []const u8 {
    zig {
        return val;
    }
}

fn valuesEqual(a: auto, b: auto): bool {
    zig {
        const TA = @TypeOf(a);
        const TB = @TypeOf(b);
        if (comptime isStringLikeType(TA) and isStringLikeType(TB)) {
            return std.mem.eql(u8, asByteSlice(a), asByteSlice(b));
        }
        return __cp_rt.cp_eq(a, b);
    }
}

fn valuesDeepEqual(a: auto, b: auto): bool {
    zig {
        const TA = @TypeOf(a);
        const TB = @TypeOf(b);
        if (comptime isStringLikeType(TA) and isStringLikeType(TB)) {
            return std.mem.eql(u8, asByteSlice(a), asByteSlice(b));
        }
        const tiA = @typeInfo(TA);
        const tiB = @typeInfo(TB);
        if ((tiA == .pointer and tiA.pointer.size == .slice) and
            (tiB == .pointer and tiB.pointer.size == .slice))
        {
            if (a.len != b.len) return false;
            for (a, b) |ea, eb| {
                if (!valuesDeepEqual(ea, eb)) return false;
            }
            return true;
        }
        if (tiA == .array and tiB == .array) {
            if (tiA.array.len != tiB.array.len) return false;
            for (a, b) |ea, eb| {
                if (!valuesDeepEqual(ea, eb)) return false;
            }
            return true;
        }
        if (tiA == .array and (tiB == .pointer and tiB.pointer.size == .slice)) {
            if (a.len != b.len) return false;
            for (a, b) |ea, eb| {
                if (!valuesDeepEqual(ea, eb)) return false;
            }
            return true;
        }
        if ((tiA == .pointer and tiA.pointer.size == .slice) and tiB == .array) {
            if (a.len != b.len) return false;
            for (a, b) |ea, eb| {
                if (!valuesDeepEqual(ea, eb)) return false;
            }
            return true;
        }
        if (tiA == .optional and tiB == .optional) {
            if (a == null and b == null) return true;
            if (a == null or b == null) return false;
            return valuesDeepEqual(a.?, b.?);
        }
        if (tiA == .@"struct" and TB == TA) {
            inline for (tiA.@"struct".fields) |f| {
                if (!valuesDeepEqual(@field(a, f.name), @field(b, f.name))) return false;
            }
            return true;
        }
        return __cp_rt.cp_eq(a, b);
    }
}

fn evalTruthy(val: auto): bool {
    zig {
        const T = @TypeOf(val);
        const ti = @typeInfo(T);
        if (T == bool) return val;
        if (ti == .int or ti == .comptime_int) return val != 0;
        if (ti == .float or ti == .comptime_float) return val != 0.0 and !std.math.isNan(val);
        if (ti == .optional) return val != null;
        if (comptime isStringLikeType(T)) return asByteSlice(val).len > 0;
        if (ti == .pointer and ti.pointer.size == .slice) return val.len > 0;
        if (ti == .array) return ti.array.len > 0;
        return true;
    }
}

fn evalIsNull(val: auto): bool {
    zig {
        const T = @TypeOf(val);
        if (T == @TypeOf(null)) return true;
        const ti = @typeInfo(T);
        if (ti == .optional) return val == null;
        return false;
    }
}

fn evalContains(actual: auto, item: auto): bool {
    zig {
        const TA = @TypeOf(actual);
        const TI = @TypeOf(item);
        if (comptime isStringLikeType(TA) and isStringLikeType(TI)) {
            return includesSubstring(asByteSlice(actual), asByteSlice(item));
        }
        const tiA = @typeInfo(TA);
        if ((tiA == .pointer and tiA.pointer.size == .slice) or tiA == .array) {
            for (actual) |elem| {
                if (valuesDeepEqual(elem, item)) return true;
            }
            return false;
        }
        @compileError("toContain() requires a string, slice, or array");
    }
}

fn evalLength(actual: auto): usize {
    zig {
        const T = @TypeOf(actual);
        if (comptime isStringLikeType(T)) return asByteSlice(actual).len;
        const ti = @typeInfo(T);
        if (ti == .pointer and ti.pointer.size == .slice) return actual.len;
        if (ti == .array) return ti.array.len;
        if (ti == .@"struct") {
            if (@hasField(T, "len")) return @as(usize, @intCast(actual.len));
            if (@hasDecl(T, "len")) return @as(usize, @intCast(actual.len()));
        }
        if (ti == .pointer and ti.pointer.size == .one) {
            const C = ti.pointer.child;
            const cti = @typeInfo(C);
            if (cti == .@"struct") {
                if (@hasField(C, "len")) return @as(usize, @intCast(actual.len));
                if (@hasDecl(C, "len")) return @as(usize, @intCast(actual.len()));
            }
        }
        @compileError("toHaveLength() requires a string, slice, array, or struct with .len");
    }
}

fn toF64(val: auto): f64 {
    zig {
        const T = @TypeOf(val);
        const ti = @typeInfo(T);
        if (ti == .float or ti == .comptime_float) return @as(f64, @floatCast(val));
        if (ti == .int or ti == .comptime_int) return @as(f64, @floatFromInt(val));
        @compileError("Numeric matcher requires an int or float value");
    }
}

fn evalThrows(actual: auto): bool {
    zig {
        const T = @TypeOf(actual);
        const ti = @typeInfo(T);
        if (ti == .error_union) {
            if (actual) |_| {
                return false;
            } else |_| {
                return true;
            }
        }
        if (ti == .@"fn" or (ti == .pointer and ti.pointer.size == .one and @typeInfo(ti.pointer.child) == .@"fn")) {
            const FnT = if (ti == .pointer) ti.pointer.child else T;
            const fn_info = @typeInfo(FnT).@"fn";
            if (fn_info.params.len == 0) {
                const res = actual();
                if (@typeInfo(@TypeOf(res)) == .error_union) {
                    if (res) |_| return false else |_| return true;
                }
                return false;
            }
            if (fn_info.params.len == 1) {
                const P0 = fn_info.params[0].type orelse @compileError("Unsupported callback param");
                const p0_ti = @typeInfo(P0);
                if (p0_ti == .pointer and p0_ti.pointer.size == .one) {
                    const Ctx = p0_ti.pointer.child;
                    if (@typeInfo(Ctx) == .@"struct" and @hasField(Ctx, "pc") and @hasField(Ctx, "arena")) {
                        var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
                        defer arena.deinit();
                        var ctx = std.mem.zeroes(Ctx);
                        ctx.arena = &arena;
                        const res = actual(&ctx);
                        if (@typeInfo(@TypeOf(res)) == .error_union) {
                            if (res) |_| return false else |_| return true;
                        }
                        return false;
                    }
                }
            }
        }
        @compileError("toThrow() requires a 0-arg function, arrow lambda, or error union");
    }
}

/// Shared assertion dispatcher parameterized by `negated: bool` to avoid duplicating
/// matcher logic between `Expectation(T)` and `NegatedExpectation(T)`.
fn ExpectCore(comptime T: type, comptime negated: bool): type {
    return struct {
        const Self = @This();
        actual: T,

        static fn finish(ok: bool, label: []const u8, expected: auto, actual: auto) {
            const passed = if (negated) !ok else ok;
            if (!passed) {
                failWithExpectedGot(label, negated, expected, actual);
            }
        }

        static fn finishMsg(ok: bool, label: []const u8, msg: []const u8) {
            const passed = if (negated) !ok else ok;
            if (!passed) {
                reportExpectFailure(label, msg);
            }
        }

        pub fn toBe(expected: auto) {
            Self.finish(valuesEqual(_.actual, expected), "toBe", expected, _.actual);
        }

        pub fn toEqual(expected: auto) {
            Self.finish(valuesDeepEqual(_.actual, expected), "toEqual", expected, _.actual);
        }

        pub fn toStrictEqual(expected: auto) {
            Self.finish(valuesDeepEqual(_.actual, expected), "toStrictEqual", expected, _.actual);
        }

        pub fn toContain(item: auto) {
            Self.finish(evalContains(_.actual, item), "toContain", item, _.actual);
        }

        pub fn toBeTruthy() {
            Self.finishMsg(evalTruthy(_.actual), "toBeTruthy", if (negated) "expected falsy value" else "expected truthy value");
        }

        pub fn toBeFalsy() {
            Self.finishMsg(!evalTruthy(_.actual), "toBeFalsy", if (negated) "expected truthy value" else "expected falsy value");
        }

        pub fn toBeNull() {
            Self.finishMsg(evalIsNull(_.actual), "toBeNull", if (negated) "expected non-null value" else "expected null");
        }

        pub fn toBeDefined() {
            Self.finishMsg(!evalIsNull(_.actual), "toBeDefined", if (negated) "expected null/undefined" else "expected defined value");
        }

        pub fn toBeUndefined() {
            Self.finishMsg(evalIsNull(_.actual), "toBeUndefined", if (negated) "expected defined value" else "expected undefined/null");
        }

        pub fn toBeGreaterThan(expected: auto) {
            Self.finish(toF64(_.actual) > toF64(expected), "toBeGreaterThan", expected, _.actual);
        }

        pub fn toBeGreaterThanOrEqual(expected: auto) {
            Self.finish(toF64(_.actual) >= toF64(expected), "toBeGreaterThanOrEqual", expected, _.actual);
        }

        pub fn toBeLessThan(expected: auto) {
            Self.finish(toF64(_.actual) < toF64(expected), "toBeLessThan", expected, _.actual);
        }

        pub fn toBeLessThanOrEqual(expected: auto) {
            Self.finish(toF64(_.actual) <= toF64(expected), "toBeLessThanOrEqual", expected, _.actual);
        }

        pub fn toBeCloseTo(expected: f64, num_digits: i32 = 2) {
            Self.finish(isCloseTo(toF64(_.actual), expected, num_digits), "toBeCloseTo", expected, _.actual);
        }

        pub fn toBeNaN() {
            Self.finishMsg(isNaNF64(toF64(_.actual)), "toBeNaN", if (negated) "expected non-NaN" else "expected NaN");
        }

        pub fn toBeFinite() {
            Self.finishMsg(isFiniteF64(toF64(_.actual)), "toBeFinite", if (negated) "expected non-finite" else "expected finite number");
        }

        pub fn toBePositive() {
            Self.finishMsg(isPositiveF64(toF64(_.actual)), "toBePositive", if (negated) "expected non-positive" else "expected positive number");
        }

        pub fn toBeNegative() {
            Self.finishMsg(isNegativeF64(toF64(_.actual)), "toBeNegative", if (negated) "expected non-negative" else "expected negative number");
        }

        pub fn toHaveLength(expected_len: usize) {
            const actual_len = evalLength(_.actual);
            Self.finish(actual_len == expected_len, "toHaveLength", expected_len, actual_len);
        }

        pub fn toMatch(pattern: []const u8) {
            const hay = asByteSlice(_.actual);
            Self.finish(includesSubstring(hay, pattern), "toMatch", pattern, hay);
        }

        pub fn toStartWith(prefix: []const u8) {
            const hay = asByteSlice(_.actual);
            Self.finish(std.mem.startsWith(u8, hay, prefix), "toStartWith", prefix, hay);
        }

        pub fn toEndWith(suffix: []const u8) {
            const hay = asByteSlice(_.actual);
            Self.finish(std.mem.endsWith(u8, hay, suffix), "toEndWith", suffix, hay);
        }

        pub fn toThrow() {
            Self.finishMsg(evalThrows(_.actual), "toThrow", if (negated) "expected function not to throw" else "expected function to throw");
        }

        pub fn toHaveBeenCalled() {
            Self.finishMsg(_.actual.toHaveBeenCalled(), "toHaveBeenCalled", if (negated) "expected mock not to be called" else "expected mock to be called");
        }

        pub fn toHaveBeenCalledOnce() {
            Self.finish(_.actual.toHaveBeenCalledOnce(), "toHaveBeenCalledOnce", @as(u64, 1), _.actual.callCount());
        }

        pub fn toHaveBeenCalledTimes(expected: u64) {
            Self.finish(_.actual.toHaveBeenCalledTimes(expected), "toHaveBeenCalledTimes", expected, _.actual.callCount());
        }

        pub fn toHaveBeenCalledWith(arg_tag: i64, arg_str: []const u8) {
            Self.finishMsg(_.actual.toHaveBeenCalledWith(arg_tag, arg_str), "toHaveBeenCalledWith", arg_str);
        }

        pub fn toHaveReturned() {
            Self.finishMsg(_.actual.toHaveReturned(), "toHaveReturned", if (negated) "expected mock not to have returned" else "expected mock to have returned");
        }

        pub fn toHaveReturnedTimes(expected: u64) {
            Self.finish(_.actual.toHaveReturnedTimes(expected), "toHaveReturnedTimes", expected, _.actual.returned_calls);
        }
    };
}

/// Negated expectation handle exposed as `expect(actual).not`.
pub fn NegatedExpectation(comptime T: type): type {
    return ExpectCore(T, true);
}

/// Chainable Bun/Jest-compatible expectation handle returned by `expect(actual)`.
pub fn Expectation(comptime T: type): type {
    const Core = ExpectCore(T, false);
    const Neg = NegatedExpectation(T);
    return struct {
        const Self = @This();
        actual: T,
        not: Neg,

        pub fn toBe(expected: auto) {
            (Core{ actual: _.actual }).toBe(expected);
        }

        pub fn toEqual(expected: auto) {
            (Core{ actual: _.actual }).toEqual(expected);
        }

        pub fn toStrictEqual(expected: auto) {
            (Core{ actual: _.actual }).toStrictEqual(expected);
        }

        pub fn toContain(item: auto) {
            (Core{ actual: _.actual }).toContain(item);
        }

        pub fn toBeTruthy() {
            (Core{ actual: _.actual }).toBeTruthy();
        }

        pub fn toBeFalsy() {
            (Core{ actual: _.actual }).toBeFalsy();
        }

        pub fn toBeNull() {
            (Core{ actual: _.actual }).toBeNull();
        }

        pub fn toBeDefined() {
            (Core{ actual: _.actual }).toBeDefined();
        }

        pub fn toBeUndefined() {
            (Core{ actual: _.actual }).toBeUndefined();
        }

        pub fn toBeGreaterThan(expected: auto) {
            (Core{ actual: _.actual }).toBeGreaterThan(expected);
        }

        pub fn toBeGreaterThanOrEqual(expected: auto) {
            (Core{ actual: _.actual }).toBeGreaterThanOrEqual(expected);
        }

        pub fn toBeLessThan(expected: auto) {
            (Core{ actual: _.actual }).toBeLessThan(expected);
        }

        pub fn toBeLessThanOrEqual(expected: auto) {
            (Core{ actual: _.actual }).toBeLessThanOrEqual(expected);
        }

        pub fn toBeCloseTo(expected: f64, num_digits: i32 = 2) {
            (Core{ actual: _.actual }).toBeCloseTo(expected, num_digits);
        }

        pub fn toBeNaN() {
            (Core{ actual: _.actual }).toBeNaN();
        }

        pub fn toBeFinite() {
            (Core{ actual: _.actual }).toBeFinite();
        }

        pub fn toBePositive() {
            (Core{ actual: _.actual }).toBePositive();
        }

        pub fn toBeNegative() {
            (Core{ actual: _.actual }).toBeNegative();
        }

        pub fn toHaveLength(expected_len: usize) {
            (Core{ actual: _.actual }).toHaveLength(expected_len);
        }

        pub fn toMatch(pattern: []const u8) {
            (Core{ actual: _.actual }).toMatch(pattern);
        }

        pub fn toStartWith(prefix: []const u8) {
            (Core{ actual: _.actual }).toStartWith(prefix);
        }

        pub fn toEndWith(suffix: []const u8) {
            (Core{ actual: _.actual }).toEndWith(suffix);
        }

        pub fn toThrow() {
            (Core{ actual: _.actual }).toThrow();
        }

        pub fn toHaveBeenCalled() {
            (Core{ actual: _.actual }).toHaveBeenCalled();
        }

        pub fn toHaveBeenCalledOnce() {
            (Core{ actual: _.actual }).toHaveBeenCalledOnce();
        }

        pub fn toHaveBeenCalledTimes(expected: u64) {
            (Core{ actual: _.actual }).toHaveBeenCalledTimes(expected);
        }

        pub fn toHaveBeenCalledWith(arg_tag: i64, arg_str: []const u8) {
            (Core{ actual: _.actual }).toHaveBeenCalledWith(arg_tag, arg_str);
        }

        pub fn toHaveReturned() {
            (Core{ actual: _.actual }).toHaveReturned();
        }

        pub fn toHaveReturnedTimes(expected: u64) {
            (Core{ actual: _.actual }).toHaveReturnedTimes(expected);
        }
    };
}

/// Creates a chainable Bun/Jest-style `Expectation` for `actual`.
pub fn expect(actual: auto): Expectation(NormalizeExpectType(@TypeOf(actual))) {
    const OutT = NormalizeExpectType(@TypeOf(actual));
    zig {
        const coerced: OutT = if (@TypeOf(actual) == @TypeOf(null)) null else actual;
        return Expectation(OutT){
            .actual = coerced,
            .not = NegatedExpectation(OutT){ .actual = coerced },
        };
    }
}
