// Source: cp/cap/test/test_junit_production_deep.cp (Production deep tests for cap:junit)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    JunitOutcome,
    JunitSuite,
    JunitCase,
    JunitSummary,
    JunitSuiteIter,
    JunitCaseIter,
    isJunitXml,
    summarizeJunit,
} from "cap:obs";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff < eps;
}

const SAMPLE_MULTI_SUITE_XML: []const u8 =
    "\xEF\xBB\xBF" ++
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
    "<!-- CI JUnit report -->\n" ++
    "<testsuites name=\"cp-ci\" tests=\"5\" failures=\"1\" errors=\"1\" skipped=\"1\">\n" ++
    "  <testsuite name=\"auth.suite\" tests=\"4\" failures=\"1\" errors=\"1\" skipped=\"1\" time=\"1.500\" timestamp=\"2026-11-21T10:00:00Z\">\n" ++
    "    <testcase classname=\"auth.TokenTest\" name=\"testSignValid\" time=\"0.120\"/>\n" ++
    "    <testcase classname=\"auth.TokenTest\" name=\"testVerifyExpired\" time=\"0.380\">\n" ++
    "      <failure message=\"expected 401, got 200\" type=\"AssertionError\"><![CDATA[at auth/token.cp:42\nexpected <401> but was <200>]]></failure>\n" ++
    "      <system-out>debug log line</system-out>\n" ++
    "    </testcase>\n" ++
    "    <testcase classname=\"auth.DbTest\" name=\"testConnect\" time=\"0.950\">\n" ++
    "      <error message=\"connection refused\" type=\"IoError\">panic: dial tcp 127.0.0.1:5432</error>\n" ++
    "    </testcase>\n" ++
    "    <testcase classname=\"auth.SsoTest\" name=\"testOidcLive\" time=\"0.000\">\n" ++
    "      <skipped message=\"requires live tenant\"/>\n" ++
    "    </testcase>\n" ++
    "  </testsuite>\n" ++
    "  <testsuite name=\"empty.suite\" tests=\"0\" time=\"0.000\"/>\n" ++
    "  <testsuite name=\"math.suite\" tests=\"1\" time=\"0.250\">\n" ++
    "    <testcase classname=\"math.VecTest\" name=\"testDot\" time=\"0.250\">\n" ++
    "      <system-out>ok</system-out>\n" ++
    "    </testcase>\n" ++
    "  </testsuite>\n" ++
    "</testsuites>\n";

const SAMPLE_SINGLE_SUITE_GREEN: []const u8 =
    "<?xml version='1.0' encoding='utf-8'?>\n" ++
    "<testsuite name='pytest' tests='2' failures='0' errors='0' skipped='1'>\n" ++
    "  <testcase classname='pkg.mod' name='test_fast' time='0.05'/>\n" ++
    "  <testcase classname='pkg.mod' name='test_skip' time='0.00'><skipped message='os only'/></testcase>\n" ++
    "</testsuite>\n";

test("1. Multi-testsuite <testsuites> parsing with BOM, comments, and self-closing empty suite", () => {
    expect_eq(isJunitXml(SAMPLE_MULTI_SUITE_XML), true);

    let sit = JunitSuiteIter.init(SAMPLE_MULTI_SUITE_XML);

    const s1 = sit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, s1.name, "auth.suite"), true);
    expect_eq(s1.tests, 4);
    expect_eq(s1.failures, 1);
    expect_eq(s1.errors, 1);
    expect_eq(s1.skipped, 1);
    expect_eq(approxEq(s1.time_sec, 1.5, 0.001), true);
    expect_eq(std.mem.eql(u8, s1.timestamp, "2026-11-21T10:00:00Z"), true);

    // 自闭合空 testsuite
    const s2 = sit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, s2.name, "empty.suite"), true);
    expect_eq(s2.tests, 0);
    let c2 = s2.cases();
    expect_eq(c2.next() == null, true);

    const s3 = sit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, s3.name, "math.suite"), true);
    expect_eq(sit.next() == null, true);
    expect_eq(sit.malformed, false);
});

test("2. Single top-level <testsuite> with single-quoted attributes & allPassed() == true", () => {
    expect_eq(isJunitXml(SAMPLE_SINGLE_SUITE_GREEN), true);
    const sum = summarizeJunit(SAMPLE_SINGLE_SUITE_GREEN);
    expect_eq(sum.suite_count, 1);
    expect_eq(sum.total, 2);
    expect_eq(sum.passed, 1);
    expect_eq(sum.skipped, 1);
    expect_eq(sum.failed, 0);
    expect_eq(sum.errored, 0);
    expect_eq(sum.allPassed(), true);
    // suite 无 time 属性时自动回退累加 case 耗时 (0.05)
    expect_eq(approxEq(sum.total_time_sec, 0.05, 0.001), true);
});

test("3. Four testcase outcomes (passed/failed/errored/skipped) & CDATA stacktrace unwrapping", () => {
    let sit = JunitSuiteIter.init(SAMPLE_MULTI_SUITE_XML);
    const s1 = sit.next() ?? unreachable;
    let cit = s1.cases();

    // 1. passed (self-closing)
    const tc1 = cit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, tc1.name, "testSignValid"), true);
    expect_eq(std.mem.eql(u8, tc1.classname, "auth.TokenTest"), true);
    expect_eq(approxEq(tc1.time_sec, 0.12, 0.001), true);
    expect_eq(tc1.outcome == JunitOutcome.passed, true);

    // 2. failed (with CDATA detail_text + message + type)
    const tc2 = cit.next() ?? unreachable;
    expect_eq(tc2.outcome == JunitOutcome.failed, true);
    expect_eq(std.mem.eql(u8, tc2.message, "expected 401, got 200"), true);
    expect_eq(std.mem.eql(u8, tc2.fail_type, "AssertionError"), true);
    expect_eq(std.mem.eql(u8, tc2.detail_text, "at auth/token.cp:42\nexpected <401> but was <200>"), true);

    // 3. errored (plain text detail)
    const tc3 = cit.next() ?? unreachable;
    expect_eq(tc3.outcome == JunitOutcome.errored, true);
    expect_eq(std.mem.eql(u8, tc3.message, "connection refused"), true);
    expect_eq(std.mem.eql(u8, tc3.fail_type, "IoError"), true);
    expect_eq(std.mem.eql(u8, tc3.detail_text, "panic: dial tcp 127.0.0.1:5432"), true);

    // 4. skipped (self-closing <skipped .../>)
    const tc4 = cit.next() ?? unreachable;
    expect_eq(tc4.outcome == JunitOutcome.skipped, true);
    expect_eq(std.mem.eql(u8, tc4.message, "requires live tenant"), true);

    expect_eq(cit.next() == null, true);
});

test("4. summarizeJunit aggregates multi-suite totals, outcomes, time, and allPassed() == false", () => {
    const sum = summarizeJunit(SAMPLE_MULTI_SUITE_XML);
    expect_eq(sum.suite_count, 3);
    expect_eq(sum.total, 5);
    expect_eq(sum.passed, 2);
    expect_eq(sum.failed, 1);
    expect_eq(sum.errored, 1);
    expect_eq(sum.skipped, 1);
    expect_eq(approxEq(sum.total_time_sec, 1.75, 0.001), true);
    expect_eq(sum.allPassed(), false);
});

test("5. Resilience against non-JUnit XML, malformed numeric attrs, and unclosed tags", () => {
    expect_eq(isJunitXml(""), false);
    expect_eq(isJunitXml("<html><body>not junit</body></html>"), false);
    expect_eq(isJunitXml("{\"not\": \"xml\"}"), false);

    // 畸形数字属性安全降级为 0
    const bad_num = "<testsuite name=\"s\" tests=\"NaN\" time=\"bad\"><testcase name=\"t\" time=\"xxx\"/></testsuite>";
    let sit1 = JunitSuiteIter.init(bad_num);
    const s = sit1.next() ?? unreachable;
    expect_eq(s.tests, 0);
    expect_eq(approxEq(s.time_sec, 0.0, 0.001), true);

    // 未闭合 <testsuite> 标记 malformed
    const unclosed_suite = "<testsuite name=\"broken\"><testcase name=\"a\"/>";
    let sit2 = JunitSuiteIter.init(unclosed_suite);
    expect_eq(sit2.next() == null, true);
    expect_eq(sit2.malformed, true);

    // 未闭合 <testcase> 标记 malformed
    const unclosed_case = "<testcase name=\"broken\"><failure>oops";
    let cit = JunitCaseIter.init(unclosed_case);
    expect_eq(cit.next() == null, true);
    expect_eq(cit.malformed, true);
});
