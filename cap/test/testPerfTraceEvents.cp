// cap/test/testPerfTraceEvents.cp
// Production-grade deep tests for cap:perf traceEvents.cp
// (Canonical 59 PerfTraceEvent catalog, 64-bit PerfTraceFilter & zero-alloc PerfTraceAccumulator)

import { test, assert, assertEq } from "cap:test";
import {
    PERF_TRACE_EVENT_COUNT,
    PERF_TRACE_ALL_MASK,
    PerfTraceCategory,
    PerfTraceEvent,
    categoryEventMask,
    PerfTraceFilter,
    PerfTraceAccumulator,
} from "cap:perf";

fn startsWith(haystack: []const u8, prefix: []const u8): bool {
    if (prefix.len > haystack.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (haystack[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}

test("PerfTraceEvent catalog: all 59 events ID/name/fromName roundtrip & bounds", () => {
    assertEq(PERF_TRACE_EVENT_COUNT, 59);
    let id: u16 = 0;
    let union_bits: u64 = 0;
    while (id < PERF_TRACE_EVENT_COUNT) {
        if (ev := PerfTraceEvent.fromId(id)) {
            assertEq(ev.toId(), id);
            const ev_name = ev.name();
            assert(ev_name.len > 0);
            if (parsed := PerfTraceEvent.fromName(ev_name)) {
                assertEq(parsed.toId(), id);
            } else {
                assert(false);
            }
            const b = ev.bit();
            assert((union_bits & b) == 0); // Every event has a unique bit
            union_bits |= b;
        } else {
            assert(false);
        }
        id += 1;
    }

    assertEq(union_bits, PERF_TRACE_ALL_MASK);
    assert(PerfTraceEvent.fromId(59) == null);
    assert(PerfTraceEvent.fromId(65535) == null);
    assert(PerfTraceEvent.fromName("") == null);
    assert(PerfTraceEvent.fromName("Bundler.NonExistent") == null);
    assert(PerfTraceEvent.fromName("bundler.parsejs") == null);
});

test("PerfTraceCategory: every event name matches its owning subsystem prefix", () => {
    let id: u16 = 0;
    while (id < PERF_TRACE_EVENT_COUNT) {
        if (ev := PerfTraceEvent.fromId(id)) {
            const cat_name = ev.categoryName();
            const ev_name = ev.name();
            assert(startsWith(ev_name, cat_name));
            assert(ev_name.len > cat_name.len);
            assertEq(ev_name[cat_name.len], '.');
        } else {
            assert(false);
        }
        id += 1;
    }

    // Spot check specific categories
    assertEq(PerfTraceEvent.BundlerParseJS.category(), PerfTraceCategory.Bundler);
    assertEq(PerfTraceEvent.ExtractTarballExtract.category(), PerfTraceCategory.ExtractTarball);
    assertEq(PerfTraceEvent.JSParserParse.category(), PerfTraceCategory.JSParser);
    assertEq(PerfTraceEvent.JSPrinterPrintWithSourceMap.category(), PerfTraceCategory.JSPrinter);
    assertEq(PerfTraceEvent.ModuleResolverResolve.category(), PerfTraceCategory.ModuleResolver);
    assertEq(PerfTraceEvent.PackageInstallerInstall.category(), PerfTraceCategory.PackageInstaller);
    assertEq(PerfTraceEvent.RuntimeTranspilerCacheSave.category(), PerfTraceCategory.RuntimeTranspilerCache);
    assertEq(PerfTraceEvent.TestCommandPrintCodeCoverageText.category(), PerfTraceCategory.TestCommand);
});

test("PerfTraceFilter: O(1) bitmask enable/disable by event and category", () => {
    let f = PerfTraceFilter.none();
    assertEq(f.enabledCount(), 0);
    assert(!f.isEnabled(PerfTraceEvent.BundlerParseJS));

    const all = PerfTraceFilter.all();
    assertEq(all.enabledCount(), 59);
    assert(all.isEnabled(PerfTraceEvent.BundlerBindImportsToExports));
    assert(all.isEnabled(PerfTraceEvent.TestCommandPrintCodeCoverageText));

    f.enableCategory(PerfTraceCategory.JSParser);
    assertEq(f.enabledCount(), 4);
    assert(f.isEnabled(PerfTraceEvent.JSParserAnalyze));
    assert(f.isEnabled(PerfTraceEvent.JSParserParse));
    assert(f.isEnabled(PerfTraceEvent.JSParserPostvisit));
    assert(f.isEnabled(PerfTraceEvent.JSParserVisit));
    assert(!f.isEnabled(PerfTraceEvent.JSPrinterPrint));

    f.disable(PerfTraceEvent.JSParserVisit);
    assertEq(f.enabledCount(), 3);
    assert(!f.isEnabled(PerfTraceEvent.JSParserVisit));

    f.enable(PerfTraceEvent.ModuleResolverResolve);
    assertEq(f.enabledCount(), 4);
    assert(f.isEnabled(PerfTraceEvent.ModuleResolverResolve));

    f.disableCategory(PerfTraceCategory.JSParser);
    assertEq(f.enabledCount(), 1);
    assert(f.isEnabled(PerfTraceEvent.ModuleResolverResolve));

    // Bundler category has 33 events (0..=32)
    const bundler_mask = categoryEventMask(PerfTraceCategory.Bundler);
    const bf = PerfTraceFilter.forCategory(PerfTraceCategory.Bundler);
    assertEq(bf.mask, bundler_mask);
    assertEq(bf.enabledCount(), 33);
});

test("PerfTraceAccumulator: record, recordFiltered, averages and peaks", () => {
    let acc = PerfTraceAccumulator.init();
    assertEq(acc.countOf(PerfTraceEvent.JSParserParse), 0);
    assertEq(acc.avgNsOf(PerfTraceEvent.JSParserParse), 0);
    assert(acc.hottestEvent() == null);

    acc.record(PerfTraceEvent.JSParserParse, 100_000);
    acc.record(PerfTraceEvent.JSParserParse, 300_000);
    acc.record(PerfTraceEvent.JSParserParse, 200_000);

    assertEq(acc.countOf(PerfTraceEvent.JSParserParse), 3);
    assertEq(acc.totalNsOf(PerfTraceEvent.JSParserParse), 600_000);
    assertEq(acc.maxNsOf(PerfTraceEvent.JSParserParse), 300_000);
    assertEq(acc.avgNsOf(PerfTraceEvent.JSParserParse), 200_000);

    // Filtered recording ignores disabled events
    const filter = PerfTraceFilter.forCategory(PerfTraceCategory.JSPrinter);
    acc.recordFiltered(filter, PerfTraceEvent.BundlerReadFile, 999_999);
    acc.recordFiltered(filter, PerfTraceEvent.JSPrinterPrint, 150_000);
    assertEq(acc.countOf(PerfTraceEvent.BundlerReadFile), 0);
    assertEq(acc.countOf(PerfTraceEvent.JSPrinterPrint), 1);
    assertEq(acc.totalNsOf(PerfTraceEvent.JSPrinterPrint), 150_000);
});

test("PerfTraceAccumulator: categoryTotalNs, grandTotalNs, hottestEvent & reset", () => {
    let acc = PerfTraceAccumulator.init();
    acc.record(PerfTraceEvent.BundlerReadFile, 40_000);
    acc.record(PerfTraceEvent.BundlerParseJS, 120_000);
    acc.record(PerfTraceEvent.JSParserParse, 90_000);
    acc.record(PerfTraceEvent.JSParserVisit, 50_000);
    acc.record(PerfTraceEvent.TestCommandPrintCodeCoverageLCov, 250_000);

    assertEq(acc.categoryTotalNs(PerfTraceCategory.Bundler), 160_000);
    assertEq(acc.categoryTotalNs(PerfTraceCategory.JSParser), 140_000);
    assertEq(acc.categoryTotalNs(PerfTraceCategory.TestCommand), 250_000);
    assertEq(acc.categoryTotalNs(PerfTraceCategory.Symbols), 0);
    assertEq(acc.grandTotalNs(), 550_000);

    if (hot := acc.hottestEvent()) {
        assertEq(hot, PerfTraceEvent.TestCommandPrintCodeCoverageLCov);
    } else {
        assert(false);
    }

    acc.reset();
    assertEq(acc.grandTotalNs(), 0);
    assertEq(acc.countOf(PerfTraceEvent.BundlerParseJS), 0);
    assertEq(acc.maxNsOf(PerfTraceEvent.TestCommandPrintCodeCoverageLCov), 0);
    assert(acc.hottestEvent() == null);
});

test("PerfTraceAccumulator: saturating addition guards against u64 overflow", () => {
    let acc = PerfTraceAccumulator.init();
    const max_u64: u64 = 18446744073709551615;
    acc.record(PerfTraceEvent.BundlerWriteOutputFilesToDisk, max_u64 - 10);
    acc.record(PerfTraceEvent.BundlerWriteOutputFilesToDisk, 100);
    assertEq(acc.totalNsOf(PerfTraceEvent.BundlerWriteOutputFilesToDisk), max_u64);
    assertEq(acc.countOf(PerfTraceEvent.BundlerWriteOutputFilesToDisk), 2);
    assertEq(acc.maxNsOf(PerfTraceEvent.BundlerWriteOutputFilesToDisk), max_u64 - 10);
});
