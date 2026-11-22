import std from "std";
import { test, expect_eq } from "cap:test";
import {
    JUnitTotals,
    JUnitMergeResult,
    CoverageThreshold,
    MergedCoverageFile,
    CoverageAggregator,
    parseJUnitAttrU32,
    appendXmlEscaped,
    mergeJUnitFragments,
} from "cap:test_runner";

test("1. parseJUnitAttrU32 extracts numeric attributes and rejects malformed inputs", () => {
    const head = "<testsuites name=\"bun test\" tests=\"12\" failures=\"2\" skipped=\"3\" time=\"0.42\">";
    expect_eq(parseJUnitAttrU32(head, "tests"), u32(12));
    expect_eq(parseJUnitAttrU32(head, "failures"), u32(2));
    expect_eq(parseJUnitAttrU32(head, "skipped"), u32(3));
    expect_eq(parseJUnitAttrU32(head, "missing"), u32(0));
    expect_eq(parseJUnitAttrU32(head, "name"), u32(0));
    expect_eq(parseJUnitAttrU32("", "tests"), u32(0));
    expect_eq(parseJUnitAttrU32("<testsuites tests=\"abc\">", "tests"), u32(0));
    expect_eq(parseJUnitAttrU32("<testsuites tests=\"99", "tests"), u32(0));
});

test("2. appendXmlEscaped escapes all XML 1.0 special characters", () => {
    const alloc = std.testing.allocator;
    let out: std.ArrayListUnmanaged(u8) = .{};
    defer out.deinit(alloc);

    appendXmlEscaped(alloc, &out, "a&b <tag attr=\"x\" single='y'>") catch unreachable;
    expect_eq(
        std.mem.eql(u8, out.items, "a&amp;b &lt;tag attr=&quot;x&quot; single=&apos;y&apos;&gt;"),
        true,
    );
});

test("3. mergeJUnitFragments merges multiple worker XML fragments with exact totals", () => {
    const alloc = std.testing.allocator;
    const frag1 = "<?xml version=\"1.0\"?>\n<testsuites name=\"w0\" tests=\"3\" failures=\"0\" skipped=\"1\">\n  <testsuite name=\"a.cp\" tests=\"3\" failures=\"0\" skipped=\"1\"></testsuite>\n</testsuites>\n";
    const frag2 = "<?xml version=\"1.0\"?>\n<testsuites name=\"w1\" tests=\"5\" failures=\"2\" skipped=\"0\">\n  <testsuite name=\"b.cp\" tests=\"5\" failures=\"2\" skipped=\"0\"></testsuite>\n</testsuites>\n";

    const frags: [2][]const u8 = [frag1, frag2];
    const no_crashed: [0][]const u8 = [];

    let merged = mergeJUnitFragments(alloc, frags[0..], no_crashed[0..], 42, 1.25) catch unreachable;
    defer merged.deinit();

    expect_eq(merged.totals.tests, u32(8));
    expect_eq(merged.totals.failures, u32(2));
    expect_eq(merged.totals.skipped, u32(1));
    expect_eq(std.mem.indexOf(u8, merged.xml, "tests=\"8\" assertions=\"42\" failures=\"2\" skipped=\"1\" time=\"1.250\"") != null, true);
    expect_eq(std.mem.indexOf(u8, merged.xml, "<testsuite name=\"a.cp\"") != null, true);
    expect_eq(std.mem.indexOf(u8, merged.xml, "<testsuite name=\"b.cp\"") != null, true);
});

test("4. mergeJUnitFragments synthesizes failed suites for exited workers and skips malformed fragments", () => {
    const alloc = std.testing.allocator;
    const malformed1 = "not xml at all";
    const malformed2 = "<testsuites tests=\"99\""; // 截断无闭合标签
    const empty_body = "<testsuites tests=\"0\" failures=\"0\" skipped=\"0\"></testsuites>";

    const frags: [3][]const u8 = [malformed1, malformed2, empty_body];
    const crashed: [1][]const u8 = ["tests/a&b<1>.cp"];

    let merged = mergeJUnitFragments(alloc, frags[0..], crashed[0..], 0, -3.5) catch unreachable;
    defer merged.deinit();

    expect_eq(merged.totals.tests, u32(1));
    expect_eq(merged.totals.failures, u32(1));
    expect_eq(merged.totals.skipped, u32(0));
    // 负数耗时钳制为 0.000
    expect_eq(std.mem.indexOf(u8, merged.xml, "time=\"0.000\"") != null, true);
    // 文件名中的 & 和 < > 均应被安全 XML 转义
    expect_eq(std.mem.indexOf(u8, merged.xml, "name=\"tests/a&amp;b&lt;1&gt;.cp\"") != null, true);
    expect_eq(std.mem.indexOf(u8, merged.xml, "worker exited before reporting results") != null, true);
});

test("5. CoverageAggregator merges multi-worker LCOV with saturating hit sums, CRLF, and malformed line tolerance", () => {
    const alloc = std.testing.allocator;
    let agg = CoverageAggregator.init(alloc);
    defer agg.deinit();

    const w1 = "TN:\r\nSF:src/math.cp\r\nFNF:4\r\nFNH:2\r\nDA:1,3\r\nDA:2,0\r\nDA:3,4294967290\r\nDA:bad,line\r\nend_of_record\r\n";
    const w2 = "SF:src/math.cp\nFNF:4\nFNH:3\nDA:1,5\nDA:2,1\nDA:3,100\nDA:4,0\nend_of_record\n";

    agg.ingestLcov(w1) catch unreachable;
    agg.ingestLcov(w2) catch unreachable;

    expect_eq(agg.fileCount(), usize(1));
    const fc = agg.getFile("src/math.cp") ?? unreachable;
    expect_eq(fc.fnf, u32(4));
    expect_eq(fc.fnh, u32(3));
    expect_eq(fc.lf(), u32(4));
    expect_eq(fc.lh(), u32(3));
    expect_eq(fc.da.get(1) ?? 0, u32(8));
    expect_eq(fc.da.get(2) ?? 0, u32(1));
    // 饱和加法防止 u32 溢出回绕
    expect_eq(fc.da.get(3) ?? 0, u32(4294967295));
    expect_eq(fc.da.get(4) ?? 99, u32(0));
});

test("6. MergedCoverageFile.formatUncoveredRanges folds consecutive uncovered lines deterministically", () => {
    const alloc = std.testing.allocator;
    let agg = CoverageAggregator.init(alloc);
    defer agg.deinit();

    // 乱序插入行号，验证排序与连续区间折叠
    const lcov = "SF:src/parser.cp\nDA:10,0\nDA:5,0\nDA:4,0\nDA:6,0\nDA:1,2\nDA:2,0\nDA:7,1\nend_of_record\n" ++
        "SF:src/full.cp\nDA:1,1\nDA:2,5\nend_of_record\n";
    agg.ingestLcov(lcov) catch unreachable;

    const parser_fc = agg.getFile("src/parser.cp") ?? unreachable;
    const ranges = parser_fc.formatUncoveredRanges(alloc) catch unreachable;
    defer alloc.free(ranges);
    expect_eq(std.mem.eql(u8, ranges, "2,4-6,10"), true);

    const full_fc = agg.getFile("src/full.cp") ?? unreachable;
    const empty_ranges = full_fc.formatUncoveredRanges(alloc) catch unreachable;
    defer alloc.free(empty_ranges);
    expect_eq(empty_ranges.len, usize(0));
});

test("7. CoverageAggregator renderMergedLcov stable ordering and threshold gating", () => {
    const alloc = std.testing.allocator;
    let agg = CoverageAggregator.init(alloc);
    defer agg.deinit();

    // 空集门禁默认通过
    expect_eq(agg.evaluateThresholds(CoverageThreshold{ functions: 0.9, lines: 0.9 }), true);
    expect_eq(agg.averageLineRatio() == 1.0, true);
    expect_eq(agg.averageFuncRatio() == 1.0, true);

    // 逆序输入文件名与行号，验证稳定字典序与升序输出
    const lcov = "SF:src/z_last.cp\nFNF:2\nFNH:2\nDA:20,1\nDA:5,2\nend_of_record\n" ++
        "SF:src/a_first.cp\nFNF:2\nFNH:1\nDA:10,0\nDA:2,4\nend_of_record\n";
    agg.ingestLcov(lcov) catch unreachable;

    // z_last: 1.0/1.0, a_first: 0.5/0.5 -> 均值 0.75
    expect_eq(agg.averageFuncRatio() > 0.74 && agg.averageFuncRatio() < 0.76, true);
    expect_eq(agg.averageLineRatio() > 0.74 && agg.averageLineRatio() < 0.76, true);

    // 阈值 0.5 通过，阈值 0.8 失败（因为 a_first 为 0.5）
    expect_eq(agg.evaluateThresholds(CoverageThreshold{ functions: 0.5, lines: 0.5 }), true);
    expect_eq(agg.evaluateThresholds(CoverageThreshold{ functions: 0.8, lines: 0.5 }), false);
    expect_eq(agg.evaluateThresholds(CoverageThreshold{ functions: 0.5, lines: 0.8 }), false);

    const rendered = agg.renderMergedLcov(alloc) catch unreachable;
    defer alloc.free(rendered);

    const expected = "TN:\nSF:src/a_first.cp\nFNF:2\nFNH:1\nDA:2,4\nDA:10,0\nLF:2\nLH:1\nend_of_record\n" ++
        "TN:\nSF:src/z_last.cp\nFNF:2\nFNH:2\nDA:5,2\nDA:20,1\nLF:2\nLH:2\nend_of_record\n";
    expect_eq(std.mem.eql(u8, rendered, expected), true);
});
