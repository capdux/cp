// Regression Test for Item #2: `??` Null-Coalescing Strict Short-Circuit Evaluation
// Verifies that RHS of `a ?? b` is NEVER evaluated when `a` is non-null (including `@panic` & side effects).

let g_side_effects: i32 = 0;

fn getSome(): i32? {
    return 42;
}

fn getNone(): i32? {
    return null;
}

fn sideEffectFallback(): i32 {
    g_side_effects += 1;
    return 99;
}

fn main() {
    // 1. Non-null LHS with `@panic` on RHS must NOT panic!
    const v1: i32 = getSome() ?? @panic("RHS of ?? was eagerly evaluated!");
    if (v1 != 42) @panic("Test 1 failed: expected 42");

    // 2. Non-null LHS with side-effect RHS must NOT call RHS!
    const v2: i32 = getSome() ?? sideEffectFallback();
    if (v2 != 42 || g_side_effects != 0) @panic("Test 2 failed: sideEffectFallback should not run");

    // 3. Null LHS with side-effect RHS must evaluate RHS exactly once!
    const v3: i32 = getNone() ?? sideEffectFallback();
    if (v3 != 99 || g_side_effects != 1) @panic("Test 3 failed: expected 99 and 1 side effect");

    // 4. Literal `null ?? fallback`
    const v4: i32 = null ?? 77;
    if (v4 != 77) @panic("Test 4 failed: null ?? 77 expected 77");

    @print("PASS testNullCoalesceShortCircuit (4/4 tests)");
}
