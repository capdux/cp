// test_phase2_ergonomics.cp
// Comprehensive verification for Phase 2 ergonomic additions:
// 1. Nullish coalescing assignment (??=)
// 2. Universal fallback block (? { ... })
// 3. Design-by-Contract annotations (@pre, @ret)
// 4. Built-in vector types (f32x4, i32x4, etc.)
// 5. Default derive macro (@[Default])
// 6. Non-interfering custom .slice() method

fn check_eq(actual: anytype, expected: anytype) {
    if (actual != expected) {
        @print("FAILED: expected {any}, got {any}\n", expected, actual);
        @panic("assertion failed");
    }
}

// ----------------------------------------------------------------------------
// 1. Custom class with .slice() method to verify no disambiguation regression
// ----------------------------------------------------------------------------
class CustomParser {
    cursor: usize;

    fn init(cursor: usize) {
        _.cursor = cursor;
    }

    fn slice(start: usize): usize {
        return _.cursor + start;
    }
}

// ----------------------------------------------------------------------------
// 2. Struct with @[Default] derive macro
// ----------------------------------------------------------------------------
@[Default]
class ConfigOptions {
    port: u16;
    retries: i32;
    timeout_ms: f64;
    enable_cache: bool;
    server_name: string;
}

// ----------------------------------------------------------------------------
// 3. Functions with @pre and @ret contracts
// ----------------------------------------------------------------------------
fn computePositiveSquare(x: i32): i32
    @pre(x > 0)
    @ret(ret > 0)
{
    return x * x;
}

fn clampRatio(val: f32, max_val: f32): f32
    @pre(max_val > 0.0)
    @ret(ret <= max_val)
{
    if (val > max_val) {
        return max_val;
    }
    return val;
}

// ----------------------------------------------------------------------------
// 4. Multi-level fallbacks (? { ... })
// ----------------------------------------------------------------------------
fn tryOptional(should_fail: bool): i32? {
    if (should_fail) {
        return null;
    }
    return 42;
}

error MyErr {
    Failed,
}

fn tryNestedErrorAndOptional(mode: i32): (i32?)!MyErr {
    if (mode == 1) {
        return MyErr.Failed;
    }
    if (mode == 2) {
        return null;
    }
    return 100;
}

fn testFallbackBlock() {
    let handled_null = false;
    let v1 = tryOptional(true) ? {
        handled_null = true;
        420
    };
    check_eq(handled_null, true);
    check_eq(v1, 420);

    let v2 = tryOptional(false) ? {
        @panic("should not run");
    };
    check_eq(v2, 42);

    // Double-unwrapping: mode 1 (error)
    let handled_err = false;
    let res1 = tryNestedErrorAndOptional(1) ? {
        handled_err = true;
        999
    };
    check_eq(handled_err, true);
    check_eq(res1, 999);

    // Double-unwrapping: mode 2 (null)
    let handled_null_nest = false;
    let res2 = tryNestedErrorAndOptional(2) ? {
        handled_null_nest = true;
        888
    };
    check_eq(handled_null_nest, true);
    check_eq(res2, 888);

    // Double-unwrapping: mode 3 (success pure value)
    let res3 = tryNestedErrorAndOptional(0) ? {
        @panic("should not run");
    };
    check_eq(res3, 100);
}

// ----------------------------------------------------------------------------
// 5. Test ??=
// ----------------------------------------------------------------------------
fn testNullishAssign() {
    let opt_val: i32? = null;
    opt_val ??= 100;
    check_eq(opt_val.?, 100);

    // Should not overwrite non-null
    opt_val ??= 200;
    check_eq(opt_val.?, 100);
}

// ----------------------------------------------------------------------------
// Main Entry
// ----------------------------------------------------------------------------
export fn main() {
    @print("=== [Test 1] Nullish Coalescing Assignment (??=) ===\n");
    testNullishAssign();
    @print("✓ Nullish assignment passed\n");

    @print("=== [Test 2] Design-by-Contract (@pre, @ret) ===\n");
    let sq = computePositiveSquare(5);
    check_eq(sq, 25);
    let clamped = clampRatio(150.0, 100.0);
    check_eq(clamped, 100.0);
    @print("✓ Contract assertions passed\n");

    @print("=== [Test 3] Universal Fallback Block (? { ... }) & Double Unwrapping ===\n");
    testFallbackBlock();
    @print("✓ Universal fallback passed\n");

    @print("=== [Test 4] @[Default] Derived Macro ===\n");
    let def_cfg = ConfigOptions.default();
    check_eq(def_cfg.port, 0);
    check_eq(def_cfg.retries, 0);
    check_eq(def_cfg.enable_cache, false);
    @print("✓ Default derive passed\n");

    @print("=== [Test 5] Custom .slice() Disambiguation ===\n");
    let parser = new CustomParser(10);
    let slice_res = parser.slice(5);
    check_eq(slice_res, 15);
    @print("✓ Custom .slice() passed\n");

    @print("=== [Test 6] Native SIMD Vector Types ===\n");
    let v1: f32x4 = @splat(1.5);
    let v2: f32x4 = @splat(2.5);
    let v_sum = v1 + v2;
    check_eq(v_sum[0], 4.0);
    @print("✓ Vector types and operations passed\n");

    @print("=== ALL PHASE 2 ERGONOMIC TESTS PASSED ===\n");
}
