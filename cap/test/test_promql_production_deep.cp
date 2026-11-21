// Source: native-cp-stdlib (PromQL Lexer & AST Parser)
import std from "std";
import cap from "cap";
import { parse, parseDuration, MatchOp, AggregationOp, BinaryOp, VectorMatchCardinality } from "cap:promql";
import { test, expect_eq } from "cap:test";

test "promql instant and matrix vector selectors with matchers and offset" {
    const alloc = std.testing.allocator;

    let expr = parse(alloc, "http_requests_total{method=\"GET\",status!=\"404\",code=~\"5..\",env!~\"dev.*\"}[5m] offset 1h")?;
    defer expr.deinit(alloc);

    switch (expr) {
        .Matrix => |m| {
            expect_eq(m.range_ms, 300_000);
            expect_eq(std.mem.eql(u8, m.vector.name, "http_requests_total"), true);
            expect_eq(m.vector.offset_ms.?, 3_600_000);
            expect_eq(m.vector.matchers.len, 4);

            expect_eq(std.mem.eql(u8, m.vector.matchers[0].name, "method"), true);
            expect_eq(m.vector.matchers[0].op == MatchOp.Eq, true);
            expect_eq(std.mem.eql(u8, m.vector.matchers[0].value, "GET"), true);

            expect_eq(m.vector.matchers[1].op == MatchOp.Neq, true);
            expect_eq(m.vector.matchers[2].op == MatchOp.Regex, true);
            expect_eq(m.vector.matchers[3].op == MatchOp.NotRegex, true);
        },
        else => return error.TestUnexpectedResult,
    }

    const dur = parseDuration("1h30m")?;
    expect_eq(dur, 5_400_000);
}

test "promql function calls and nested histogram_quantile aggregation" {
    const alloc = std.testing.allocator;

    let expr = parse(alloc, "histogram_quantile(0.95, sum by (le) (rate(lat_bucket{job=\"api\"}[5m])))")?;
    defer expr.deinit(alloc);

    switch (expr) {
        .Call => |c| {
            expect_eq(std.mem.eql(u8, c.func_name, "histogram_quantile"), true);
            expect_eq(c.args.len, 2);

            switch (c.args[0]) {
                .NumberLiteral => |n| expect_eq(n > 0.94 and n < 0.96, true),
                else => return error.TestUnexpectedResult,
            }

            switch (c.args[1]) {
                .Aggregation => |agg| {
                    expect_eq(agg.op == AggregationOp.Sum, true);
                    expect_eq(agg.without, false);
                    expect_eq(agg.grouping.len, 1);
                    expect_eq(std.mem.eql(u8, agg.grouping[0], "le"), true);

                    switch (agg.expr.*) {
                        .Call => |rate_call| {
                            expect_eq(std.mem.eql(u8, rate_call.func_name, "rate"), true);
                            expect_eq(rate_call.args.len, 1);
                            switch (rate_call.args[0]) {
                                .Matrix => |mat| {
                                    expect_eq(mat.range_ms, 300_000);
                                    expect_eq(std.mem.eql(u8, mat.vector.name, "lat_bucket"), true);
                                },
                                else => return error.TestUnexpectedResult,
                            }
                        },
                        else => return error.TestUnexpectedResult,
                    }
                },
                else => return error.TestUnexpectedResult,
            }
        },
        else => return error.TestUnexpectedResult,
    }
}

test "promql binary operator precedence and vector matching modifiers" {
    const alloc = std.testing.allocator;

    // 1 + 2 * 3 ^ 2 == 19 -> Mul binds tighter than Add, Pow binds tighter than Mul
    let arith = parse(alloc, "1 + 2 * 3 ^ 2")?;
    defer arith.deinit(alloc);

    switch (arith) {
        .Binary => |top| {
            expect_eq(top.op == BinaryOp.Add, true);
            switch (top.rhs.*) {
                .Binary => |mul| {
                    expect_eq(mul.op == BinaryOp.Mul, true);
                    switch (mul.rhs.*) {
                        .Binary => |pow| expect_eq(pow.op == BinaryOp.Pow, true),
                        else => return error.TestUnexpectedResult,
                    }
                },
                else => return error.TestUnexpectedResult,
            }
        },
        else => return error.TestUnexpectedResult,
    }

    let vm = parse(alloc, "metric_a / on(job) group_left(env) metric_b > bool 0.5")?;
    defer vm.deinit(alloc);

    switch (vm) {
        .Binary => |cmp| {
            expect_eq(cmp.op == BinaryOp.Gt, true);
            expect_eq(cmp.return_bool, true);
            switch (cmp.lhs.*) {
                .Binary => |div| {
                    expect_eq(div.op == BinaryOp.Div, true);
                    const m = div.matching.?;
                    expect_eq(m.on_labels, true);
                    expect_eq(m.matching_labels.len, 1);
                    expect_eq(std.mem.eql(u8, m.matching_labels[0], "job"), true);
                    expect_eq(m.card == VectorMatchCardinality.ManyToOne, true);
                    expect_eq(m.include_labels.len, 1);
                    expect_eq(std.mem.eql(u8, m.include_labels[0], "env"), true);
                },
                else => return error.TestUnexpectedResult,
            }
        },
        else => return error.TestUnexpectedResult,
    }
}

test "promql invalid syntax interception without memory leaks" {
    const alloc = std.testing.allocator;

    const bad_inputs = [
        "",
        "   # comment only",
        "http_requests_total{code=}",
        "rate(http_requests_total[5x])",
        "sum(metric_a",
        "metric_a + * metric_b",
        "metric_a[5m",
    ];

    for (bad of bad_inputs) {
        let failed = false;
        let parsed = parse(alloc, bad) catch {
            failed = true;
            expect_eq(failed, true);
            continue;
        };
        parsed.deinit(alloc);
        expect_eq(failed, true);
    }
}

test "promql cap.promql dynamic mounting and topk aggregation" {
    const alloc = std.testing.allocator;

    let expr = cap.promql.parse(alloc, "topk(5, sum without (instance) (rate(cpu_seconds_total[1m])))")?;
    defer expr.deinit(alloc);

    switch (expr) {
        .Aggregation => |top| {
            expect_eq(top.op == AggregationOp.Topk, true);
            expect_eq(top.param != null, true);
            switch (top.expr.*) {
                .Aggregation => |inner| {
                    expect_eq(inner.op == AggregationOp.Sum, true);
                    expect_eq(inner.without, true);
                    expect_eq(inner.grouping.len, 1);
                    expect_eq(std.mem.eql(u8, inner.grouping[0], "instance"), true);
                },
                else => return error.TestUnexpectedResult,
            }
        },
        else => return error.TestUnexpectedResult,
    }
}
