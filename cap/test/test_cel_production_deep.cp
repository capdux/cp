// Source: cp/cap/test/test_cel_production_deep.cp (Deep Production Tests for cap:cel & cap.cel)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    CelError,
    CelMapEntry,
    CelValue,
    CelEnv,
    CelEvalOptions,
    celString,
    celInt,
    celBool,
    compileCel,
    compileCelWithMaxDepth,
    evalCel,
    evalCelWithOptions,
    evalBool,
    evalBoolWithOptions,
} from "cap:cel";

fn makeClaimsMap(alloc: std.mem.Allocator, role: []const u8, tier: i64): CelError!CelValue {
    let entries = alloc.alloc(CelMapEntry, 2) catch return error.OutOfMemory;
    errdefer alloc.free(entries);
    entries[0] = CelMapEntry{
        .key = alloc.dupe(u8, "role") catch return error.OutOfMemory,
        .value = try celString(alloc, role),
    };
    entries[1] = CelMapEntry{
        .key = alloc.dupe(u8, "tier") catch return error.OutOfMemory,
        .value = celInt(tier),
    };
    return CelValue{ .map_val = entries };
}

fn makeRequestEnv(alloc: std.mem.Allocator, role: []const u8, tier: i64, method: []const u8): CelError!CelEnv {
    let env = CelEnv.init();
    errdefer env.deinit(alloc);

    let claims = try makeClaimsMap(alloc, role, tier);
    errdefer claims.deinit(alloc);

    let auth_entries = alloc.alloc(CelMapEntry, 1) catch return error.OutOfMemory;
    auth_entries[0] = CelMapEntry{
        .key = alloc.dupe(u8, "claims") catch return error.OutOfMemory,
        .value = claims,
    };
    let auth_map = CelValue{ .map_val = auth_entries };

    let req_entries = alloc.alloc(CelMapEntry, 2) catch return error.OutOfMemory;
    req_entries[0] = CelMapEntry{
        .key = alloc.dupe(u8, "auth") catch return error.OutOfMemory,
        .value = auth_map,
    };
    req_entries[1] = CelMapEntry{
        .key = alloc.dupe(u8, "method") catch return error.OutOfMemory,
        .value = try celString(alloc, method),
    };
    let req_map = CelValue{ .map_val = req_entries };
    try env.bindOwned(alloc, "request", req_map);

    let roles_slice = alloc.alloc(CelValue, 2) catch return error.OutOfMemory;
    roles_slice[0] = try celString(alloc, "admin");
    roles_slice[1] = try celString(alloc, "secops");
    try env.bindOwned(alloc, "allowed_roles", CelValue{ .list_val = roles_slice });

    return env;
}

test("IAM/RBAC policy evaluation with nested map, in, short-circuit, and ternary") {
    const alloc = std.testing.allocator;
    let env = try makeRequestEnv(alloc, "admin", 3, "POST");
    defer env.deinit(alloc);

    const policy = "request.auth.claims.role in allowed_roles && (request.method == 'POST' || missing_var == 1) && (request.auth.claims.tier >= 2 ? true : false)";
    const expr = try compileCel(alloc, policy);
    defer expr.deinit(alloc);

    const allowed = try evalBool(alloc, expr, &env);
    expect_eq(allowed, true);

    // Verify false branch of short-circuit || when left is false and right has undeclared var -> should error
    const expr_short = try compileCel(alloc, "false && undeclared_xyz == 1");
    defer expr_short.deinit(alloc);
    const short_res = try evalBool(alloc, expr_short, &env);
    expect_eq(short_res, false);
}

test("String methods, has(x.y), and list/map literals") {
    const alloc = std.testing.allocator;
    let env = try makeRequestEnv(alloc, "admin", 1, "GET");
    defer env.deinit(alloc);

    const expr_str = try compileCel(alloc, "request.auth.claims.role.startsWith('ad') && request.auth.claims.role.endsWith('min') && request.auth.claims.role.contains('mi') && request.auth.claims.role.matches('^ad[a-z]+$') && size(request.auth.claims.role) == 5");
    defer expr_str.deinit(alloc);
    expect_eq(try evalBool(alloc, expr_str, &env), true);

    const expr_has = try compileCel(alloc, "has(request.auth.claims) && !has(request.auth.email) && !has(nonexistent.field)");
    defer expr_has.deinit(alloc);
    expect_eq(try evalBool(alloc, expr_has, &env), true);

    const expr_lit = try compileCel(alloc, "[10, 20, 30][1] == 20 && {'k': 'v', 'n': 42}['n'] == 42 && int('123') == 123 && string(42) == '42'");
    defer expr_lit.deinit(alloc);
    expect_eq(try evalBool(alloc, expr_lit, &env), true);
}

test("List comprehension macros: all, exists, exists_one, filter, map") {
    const alloc = std.testing.allocator;
    let env = CelEnv.init();
    defer env.deinit(alloc);

    let nums = alloc.alloc(CelValue, 4) catch return error.OutOfMemory;
    nums[0] = celInt(2);
    nums[1] = celInt(4);
    nums[2] = celInt(6);
    nums[3] = celInt(7);
    try env.bindOwned(alloc, "nums", CelValue{ .list_val = nums });

    const expr_macros = try compileCel(alloc, "nums.all(x, x > 0) && nums.exists(x, x == 7) && nums.exists_one(x, x % 2 == 1) && size(nums.filter(x, x >= 4)) == 3 && nums.map(x, x * 2)[2] == 12");
    defer expr_macros.deinit(alloc);
    expect_eq(try evalBool(alloc, expr_macros, &env), true);
}

test("Sandbox security: division by zero, type mismatch, oob, overflow, max_depth, max_steps") {
    const alloc = std.testing.allocator;
    let env = CelEnv.init();
    defer env.deinit(alloc);

    // Division by zero
    const e_div = try compileCel(alloc, "10 / 0");
    defer e_div.deinit(alloc);
    try std.testing.expectError(error.DivisionByZero, evalCel(alloc, e_div, &env));

    // Type mismatch (no implicit Int + String)
    const e_type = try compileCel(alloc, "1 + 'abc'");
    defer e_type.deinit(alloc);
    try std.testing.expectError(error.TypeMismatch, evalCel(alloc, e_type, &env));

    // Index out of bounds
    const e_oob = try compileCel(alloc, "[1, 2][5]");
    defer e_oob.deinit(alloc);
    try std.testing.expectError(error.IndexOutOfBounds, evalCel(alloc, e_oob, &env));

    // Signed integer overflow
    const e_ovf = try compileCel(alloc, "9223372036854775807 + 1");
    defer e_ovf.deinit(alloc);
    try std.testing.expectError(error.IntegerOverflow, evalCel(alloc, e_ovf, &env));

    // Max depth exceeded
    try std.testing.expectError(error.MaxDepthExceeded, compileCelWithMaxDepth(alloc, "((((1 + 2))))", 2));

    // Max steps cost limit exceeded
    const e_cost = try compileCel(alloc, "[1, 2, 3, 4, 5].all(x, x > 0)");
    defer e_cost.deinit(alloc);
    try std.testing.expectError(error.CostLimitExceeded, evalBoolWithOptions(alloc, e_cost, &env, CelEvalOptions{ .max_steps = 5 }));
}

test("cap.cel dynamic mount and std.testing.allocator zero-leak check") {
    const alloc = std.testing.allocator;
    let env = cap.cel.CelEnv.init();
    defer env.deinit(alloc);
    try env.bindOwned(alloc, "env_name", try cap.cel.celString(alloc, "prod"));

    const expr = try cap.cel.compileCel(alloc, "env_name == 'prod' && 2 + 3 * 4 == 14");
    defer expr.deinit(alloc);

    const ok = try cap.cel.evalBool(alloc, expr, &env);
    expect_eq(ok, true);
}
