// Source: Absorbed from /data/dev/bun/src/test_runner/expect/toBeFinite.cp | Synced: 2026-11-22T02:15:56Z
// Pure-CP zero-allocation IEEE-754 numeric & relational expect matchers
// (toBeFinite, toBeNaN, toBePositive, toBeNegative, toBeInteger, Object.is toBe, >, >=, <, <=).

/// IEEE-754 binary64 位掩码常量
pub const F64_SIGN_MASK: u64 = 0x8000_0000_0000_0000;
pub const F64_EXP_MASK: u64 = 0x7ff0_0000_0000_0000;
pub const F64_MANT_MASK: u64 = 0x000f_ffff_ffff_ffff;

/// IEEE-754 binary64 最大安全整数（`2^53 - 1 = 9007199254740991`）
pub const F64_MAX_SAFE_INT: f64 = 9007199254740991.0;

fn f64Bits(v: f64): u64 {
    return @as(u64, @bitCast(v));
}

/// 判定 `f64` 是否为 `NaN`（指数全 1 且尾数非 0）
pub fn isNaNF64(v: f64): bool {
    const bits = f64Bits(v);
    return (bits & F64_EXP_MASK) == F64_EXP_MASK && (bits & F64_MANT_MASK) != 0;
}

/// 判定 `f64` 是否为正无穷或负无穷
pub fn isInfiniteF64(v: f64): bool {
    const bits = f64Bits(v);
    return (bits & F64_EXP_MASK) == F64_EXP_MASK && (bits & F64_MANT_MASK) == 0;
}

/// 判定 `f64` 是否为有限数（既非 `NaN` 也非 `±Infinity`，对齐 Bun `toBeFinite`）
pub fn isFiniteF64(v: f64): bool {
    const bits = f64Bits(v);
    return (bits & F64_EXP_MASK) != F64_EXP_MASK;
}

/// 判定 `f64` 是否为 IEEE-754 负零（`-0.0`）
pub fn isNegativeZeroF64(v: f64): bool {
    return f64Bits(v) == F64_SIGN_MASK;
}

/// 判定 `f64` 是否为严格有限正数（`0 < v < +Infinity`，对齐 Bun `toBePositive`）
pub fn isPositiveF64(v: f64): bool {
    return isFiniteF64(v) && v > 0.0;
}

/// 判定 `f64` 是否为严格有限负数（`-Infinity < v < 0`，对齐 Bun `toBeNegative`）
pub fn isNegativeF64(v: f64): bool {
    return isFiniteF64(v) && v < 0.0;
}

/// 判定 `f64` 是否为精确整数（对齐 JS `Number.isInteger` 与 Bun `toBeInteger`）
pub fn isIntegerF64(v: f64): bool {
    if (!isFiniteF64(v)) {
        return false;
    }
    const bits = f64Bits(v);
    const abs_bits = bits & (~F64_SIGN_MASK);
    if (abs_bits == 0) {
        return true;
    }
    const exp_biased: i32 = @as(i32, @intCast((abs_bits >> 52) & 0x7ff));
    const exp: i32 = exp_biased - 1023;
    if (exp < 0) {
        return false;
    }
    if (exp >= 52) {
        return true;
    }
    const frac_bits: u6 = @as(u6, @intCast(52 - exp));
    const frac_mask: u64 = (@as(u64, 1) << frac_bits) - 1;
    return (abs_bits & frac_mask) == 0;
}

/// 判定 `f64` 是否为 IEEE-754 安全整数（`|v| <= 2^53 - 1` 且无小数部分）
pub fn isSafeIntegerF64(v: f64): bool {
    if (!isIntegerF64(v)) {
        return false;
    }
    const abs_v = if (v < 0.0) -v else v;
    return abs_v <= F64_MAX_SAFE_INT;
}

/// `Object.is` 语义判定（对齐 Bun `toBe`：`NaN` 与 `NaN` 相等，`+0.0` 与 `-0.0` 不相等）
pub fn isSameValueF64(a: f64, b: f64): bool {
    if (isNaNF64(a) && isNaNF64(b)) {
        return true;
    }
    return f64Bits(a) == f64Bits(b);
}

/// 浮点大于比较（含 `NaN` 保护，对齐 Bun `toBeGreaterThan`）
pub fn isGreaterThanF64(actual: f64, expected: f64): bool {
    if (isNaNF64(actual) || isNaNF64(expected)) {
        return false;
    }
    return actual > expected;
}

/// 浮点大于等于比较（含 `NaN` 保护，对齐 Bun `toBeGreaterThanOrEqual`）
pub fn isGreaterThanOrEqualF64(actual: f64, expected: f64): bool {
    if (isNaNF64(actual) || isNaNF64(expected)) {
        return false;
    }
    return actual >= expected;
}

/// 浮点小于比较（含 `NaN` 保护，对齐 Bun `toBeLessThan`）
pub fn isLessThanF64(actual: f64, expected: f64): bool {
    if (isNaNF64(actual) || isNaNF64(expected)) {
        return false;
    }
    return actual < expected;
}

/// 浮点小于等于比较（含 `NaN` 保护，对齐 Bun `toBeLessThanOrEqual`）
pub fn isLessThanOrEqualF64(actual: f64, expected: f64): bool {
    if (isNaNF64(actual) || isNaNF64(expected)) {
        return false;
    }
    return actual <= expected;
}

/// 64 位整数大于比较
pub fn isGreaterThanI64(actual: i64, expected: i64): bool {
    return actual > expected;
}

/// 64 位整数大于等于比较
pub fn isGreaterThanOrEqualI64(actual: i64, expected: i64): bool {
    return actual >= expected;
}

/// 64 位整数小于比较
pub fn isLessThanI64(actual: i64, expected: i64): bool {
    return actual < expected;
}

/// 64 位整数小于等于比较
pub fn isLessThanOrEqualI64(actual: i64, expected: i64): bool {
    return actual <= expected;
}

/// 带单次 `.not` 反转与断言计数的零分配数值匹配器
pub struct ExpectNumMatcher {
    negated: bool = false,
    assertions: usize = 0,
    passed: usize = 0,
    failed: usize = 0,

    static fn init(): ExpectNumMatcher {
        return ExpectNumMatcher{};
    }

    fn negate(*_) {
        _.negated = !_.negated;
    }

    fn record(*_, raw_pass: bool): bool {
        const ok = if (_.negated) !raw_pass else raw_pass;
        _.negated = false;
        _.assertions += 1;
        if (ok) {
            _.passed += 1;
        } else {
            _.failed += 1;
        }
        return ok;
    }

    fn toBeFinite(*_, v: f64): bool {
        return _.record(isFiniteF64(v));
    }

    fn toBeNaN(*_, v: f64): bool {
        return _.record(isNaNF64(v));
    }

    fn toBePositive(*_, v: f64): bool {
        return _.record(isPositiveF64(v));
    }

    fn toBeNegative(*_, v: f64): bool {
        return _.record(isNegativeF64(v));
    }

    fn toBeInteger(*_, v: f64): bool {
        return _.record(isIntegerF64(v));
    }

    fn toBeSafeInteger(*_, v: f64): bool {
        return _.record(isSafeIntegerF64(v));
    }

    fn toBeSameValue(*_, actual: f64, expected: f64): bool {
        return _.record(isSameValueF64(actual, expected));
    }

    fn toBeGreaterThan(*_, actual: f64, expected: f64): bool {
        return _.record(isGreaterThanF64(actual, expected));
    }

    fn toBeGreaterThanOrEqual(*_, actual: f64, expected: f64): bool {
        return _.record(isGreaterThanOrEqualF64(actual, expected));
    }

    fn toBeLessThan(*_, actual: f64, expected: f64): bool {
        return _.record(isLessThanF64(actual, expected));
    }

    fn toBeLessThanOrEqual(*_, actual: f64, expected: f64): bool {
        return _.record(isLessThanOrEqualF64(actual, expected));
    }

    fn toBeGreaterThanI64(*_, actual: i64, expected: i64): bool {
        return _.record(isGreaterThanI64(actual, expected));
    }

    fn toBeLessThanI64(*_, actual: i64, expected: i64): bool {
        return _.record(isLessThanI64(actual, expected));
    }
}
