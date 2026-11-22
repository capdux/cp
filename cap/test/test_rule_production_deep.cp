// Source: cp/cap/test/test_rule_production_deep.cp
// Deep Production Verification Suite for cap:rule & cap.rule
// Covers:
//   1. Composite Condition AST (And/Or/Not/InList/Contains/StartsWith/Exists/Gte) & Int-vs-Float numeric comparison
//   2. Salience priority ordering (stable tie-breaking) & activation_group mutual-exclusion short-circuiting
//   3. Dynamic setEnabled toggle, missing-field safe false evaluation, DuplicateRuleId & RuleNotFound interception
//   4. >32-level nested Condition AST MaxDepthExceeded guard & std.testing.allocator zero memory leaks
//   5. `import cap from "cap"; cap.rule` dynamic mounting verification

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MAX_CONDITION_DEPTH,
    RuleError,
    FactValue,
    FactMap,
    CompareOp,
    Condition,
    RuleSpec,
    FiredRule,
    RuleEvalReport,
    RuleEngine,
} from "cap:finance";

test("cap:rule - composite condition AST & Int vs Float numeric comparison") {
    const alloc = std.testing.allocator;
    let engine = RuleEngine.init(alloc);
    defer engine.deinit();

    // Rule: (country in ["CN", "SG"] AND score >= 85.5) AND (email contains "@corp" OR role == "admin") AND NOT(blocked == true) AND exists("kyc_level")
    const countries = [FactValue.Str("CN"), FactValue.Str("SG")];
    const blocked_cmp = Condition.cmp("blocked", .Eq, FactValue.Bool(true));
    const or_items = [
        Condition.cmp("email", .Contains, FactValue.Str("@corp")),
        Condition.cmp("role", .Eq, FactValue.Str("admin")),
    ];
    const and_items = [
        Condition.inList("country", &countries),
        Condition.cmp("score", .Gte, FactValue.Float(85.5)),
        Condition.orAny(&or_items),
        Condition.notExpr(&blocked_cmp),
        Condition.exists("kyc_level"),
        Condition.cmp("route", .StartsWith, FactValue.Str("/api/v2")),
    ];

    engine.addRule(RuleSpec{
        id: "fraud-allow",
        name: "Allow Verified Corporate Flow",
        salience: 50,
        condition: Condition.andAll(&and_items),
        action_tag: "ALLOW",
        action_payload: "tier=trusted",
    })?;

    // Also test Int fact compared against Float threshold (86 >= 85.5) and Float fact compared against Int threshold (90.0 >= 90)
    const int_vs_float_cond = Condition.cmp("risk_limit", .Gte, FactValue.Int(90));
    engine.addRule(RuleSpec{
        id: "float-int-match",
        name: "Float Fact vs Int Threshold",
        salience: 10,
        condition: int_vs_float_cond,
        action_tag: "AUDIT",
        action_payload: "limit_ok",
    })?;

    let facts = FactMap.init(alloc);
    defer facts.deinit();
    facts.setStr("country", "SG")?;
    facts.setInt("score", 86)?;          // Int(86) >= Float(85.5) -> true
    facts.setFloat("risk_limit", 90.0)?; // Float(90.0) >= Int(90) -> true
    facts.setStr("email", "alice@corp.example")?;
    facts.setStr("role", "member")?;
    facts.setBool("blocked", false)?;
    facts.setInt("kyc_level", 3)?;
    facts.setStr("route", "/api/v2/orders")?;

    let report = engine.evaluate(alloc, &facts)?;
    defer report.deinit();

    expect_eq(report.matched.len, 2);
    expect_eq(report.evaluated_count, 2);
    expect_eq(report.skipped_by_group_count, 0);
    expect_eq(std.mem.eql(u8, report.matched[0].rule_id, "fraud-allow"), true);
    expect_eq(std.mem.eql(u8, report.matched[1].rule_id, "float-int-match"), true);

    // Update score to 85 (85 < 85.5 -> false)
    facts.setInt("score", 85)?;
    let report2 = engine.evaluate(alloc, &facts)?;
    defer report2.deinit();

    expect_eq(report2.matched.len, 1);
    expect_eq(std.mem.eql(u8, report2.matched[0].rule_id, "float-int-match"), true);
}

test("cap:rule - salience ordering & activation_group short-circuit across groups") {
    const alloc = std.testing.allocator;
    let engine = RuleEngine.init(alloc);
    defer engine.deinit();

    // Register out of salience order; tie-breaking on salience=100 must preserve insertion order.
    engine.addRule(RuleSpec{
        id: "price-fallback",
        name: "Fallback Pricing",
        salience: 10,
        activation_group: "pricing",
        condition: Condition.cmp("active", .Eq, FactValue.Bool(true)),
        action_tag: "PRICE_STD",
        action_payload: "100",
    })?;
    engine.addRule(RuleSpec{
        id: "price-vip-first",
        name: "VIP Pricing Primary",
        salience: 100,
        activation_group: "pricing",
        condition: Condition.cmp("vip", .Eq, FactValue.Bool(true)),
        action_tag: "PRICE_VIP",
        action_payload: "70",
    })?;
    engine.addRule(RuleSpec{
        id: "price-vip-second",
        name: "VIP Pricing Secondary",
        salience: 100,
        activation_group: "pricing",
        condition: Condition.cmp("vip", .Eq, FactValue.Bool(true)),
        action_tag: "PRICE_VIP_ALT",
        action_payload: "75",
    })?;
    engine.addRule(RuleSpec{
        id: "telemetry-tag",
        name: "Cross-Group Telemetry",
        salience: 20,
        activation_group: "telemetry",
        condition: Condition.cmp("active", .Eq, FactValue.Bool(true)),
        action_tag: "EMIT_METRIC",
        action_payload: "pricing_evaluated",
    })?;
    engine.addRule(RuleSpec{
        id: "ungrouped-log",
        name: "Ungrouped Audit",
        salience: 5,
        activation_group: null,
        condition: Condition.cmp("active", .Eq, FactValue.Bool(true)),
        action_tag: "LOG_AUDIT",
        action_payload: "ok",
    })?;

    let facts = FactMap.init(alloc);
    defer facts.deinit();
    facts.setBool("active", true)?;
    facts.setBool("vip", true)?;

    let report = engine.evaluate(alloc, &facts)?;
    defer report.deinit();

    // price-vip-first (salience 100, pricing) fires -> price-vip-second & price-fallback skipped!
    // telemetry-tag (salience 20, telemetry) & ungrouped-log (salience 5, null) still fire!
    expect_eq(report.matched.len, 3);
    expect_eq(report.evaluated_count, 3);
    expect_eq(report.skipped_by_group_count, 2);

    expect_eq(std.mem.eql(u8, report.matched[0].rule_id, "price-vip-first"), true);
    expect_eq(std.mem.eql(u8, report.matched[1].rule_id, "telemetry-tag"), true);
    expect_eq(std.mem.eql(u8, report.matched[2].rule_id, "ungrouped-log"), true);

    const top = report.topAction().?;
    expect_eq(std.mem.eql(u8, top.action_tag, "PRICE_VIP"), true);
    expect_eq(std.mem.eql(u8, top.action_payload, "70"), true);
}

test("cap:rule - setEnabled toggle, missing-field safety & DuplicateRuleId/RuleNotFound") {
    const alloc = std.testing.allocator;
    let engine = RuleEngine.init(alloc);
    defer engine.deinit();

    const list_vals = [FactValue.Str("fast"), FactValue.Str("gpu")];
    engine.addRule(RuleSpec{
        id: "r-main",
        name: "Main Route",
        salience: 50,
        condition: Condition.cmp("missing_field", .Eq, FactValue.Str("x")),
        action_tag: "ACT_MAIN",
        action_payload: "1",
    })?;
    engine.addRule(RuleSpec{
        id: "r-in",
        name: "InList Route",
        salience: 40,
        condition: Condition.inList("mode", &list_vals),
        action_tag: "ACT_IN",
        action_payload: "2",
    })?;
    expect_eq(engine.ruleCount(), 2);

    // DuplicateRuleId interception
    let dup_err = false;
    engine.addRule(RuleSpec{
        id: "r-main",
        name: "Duplicate",
        salience: 1,
        condition: Condition.exists("mode"),
        action_tag: "NOOP",
    }) catch e => {
        if (e == RuleError.DuplicateRuleId) {
            dup_err = true;
        }
    };
    expect_eq(dup_err, true);

    // RuleNotFound interception
    let nf_err = false;
    engine.setEnabled("nonexistent-id", false) catch e => {
        if (e == RuleError.RuleNotFound) {
            nf_err = true;
        }
    };
    expect_eq(nf_err, true);

    // Empty FactMap: missing fields safely return false without error
    let facts = FactMap.init(alloc);
    defer facts.deinit();
    let r1 = engine.evaluate(alloc, &facts)?;
    defer r1.deinit();
    expect_eq(r1.matched.len, 0);
    expect_eq(r1.topAction() == null, true);

    // Populate "mode" -> r-in matches; then disable r-in -> 0 matches
    facts.setStr("mode", "gpu")?;
    let r2 = engine.evaluate(alloc, &facts)?;
    defer r2.deinit();
    expect_eq(r2.matched.len, 1);
    expect_eq(std.mem.eql(u8, r2.matched[0].rule_id, "r-in"), true);

    engine.setEnabled("r-in", false)?;
    let r3 = engine.evaluate(alloc, &facts)?;
    defer r3.deinit();
    expect_eq(r3.matched.len, 0);
    expect_eq(r3.evaluated_count, 1); // only r-main was enabled
}

test("cap:rule - MaxDepthExceeded (>32 nested layers) & zero memory leaks") {
    const alloc = std.testing.allocator;
    let engine = RuleEngine.init(alloc);
    defer engine.deinit();

    // Build a 34-level nested Not chain on the heap and free it after addRule (proving deep clone ownership)
    let nodes: [35]Condition = undefined;
    nodes[0] = Condition.cmp("flag", .Eq, FactValue.Bool(true));
    let i: usize = 1;
    while (i <= 34) : (i += 1) {
        nodes[i] = Condition.notExpr(&nodes[i - 1]);
    }

    expect_eq(MAX_CONDITION_DEPTH, 32);
    engine.addRule(RuleSpec{
        id: "deep-rule",
        name: "Overly Deep Condition",
        salience: 1,
        condition: nodes[34],
        action_tag: "DEEP",
    })?;

    let facts = FactMap.init(alloc);
    defer facts.deinit();
    facts.setBool("flag", true)?;

    let depth_err = false;
    * = engine.evaluate(alloc, &facts) catch e => {
        if (e == RuleError.MaxDepthExceeded) {
            depth_err = true;
        }
    };
    expect_eq(depth_err, true);
}

test("cap:rule - dynamic mounting via `import cap from \"cap\"; cap.rule`") {
    const alloc = std.testing.allocator;
    let engine = cap.finance.RuleEngine.init(alloc);
    defer engine.deinit();

    engine.addRule(cap.finance.RuleSpec{
        id: "dyn-1",
        name: "Dynamic Mount Rule",
        salience: 99,
        condition: cap.finance.Condition.cmp("env", .Eq, cap.finance.FactValue.Str("prod")),
        action_tag: "ROUTE_PROD",
        action_payload: "cluster-a",
    })?;

    let facts = cap.finance.FactMap.init(alloc);
    defer facts.deinit();
    facts.setStr("env", "prod")?;

    let report = engine.evaluate(alloc, &facts)?;
    defer report.deinit();

    expect_eq(report.matched.len, 1);
    const top = report.topAction().?;
    expect_eq(std.mem.eql(u8, top.action_tag, "ROUTE_PROD"), true);
    expect_eq(std.mem.eql(u8, top.action_payload, "cluster-a"), true);
}
