// Production deep regression tests for lib/perf/memoryCost.cp (cap:perf)

import { test, expect_eq } from "cap:test";
import {
    MemoryCostError,
    MemoryCostCategory,
    MemoryLeakVerdict,
    estimateSliceBytes,
    estimateCapacityBytes,
    MemoryCostBreakdown,
} from "cap:perf";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("memoryCost: slice/capacity estimation & category accounting", () => {
    expect_eq(estimateSliceBytes(100, 8), 800);
    expect_eq(estimateCapacityBytes(64, 16), 1024);
    expect_eq(estimateSliceBytes(0, 32), 0);

    let mc = MemoryCostBreakdown{};
    expect_eq(mc.totalBytes(), 0);
    expect_eq(mc.sharePermille(.JsCode), 0);

    mc.add(.IncrementalGraphClient, 100);
    mc.add(.IncrementalGraphServer, 200);
    mc.add(.JsCode, 300);
    mc.add(.SourceMaps, 150);
    mc.add(.Assets, 200);
    mc.add(.Other, 50);

    expect_eq(mc.totalBytes(), 1000);
    expect_eq(mc.bytesFor(.JsCode), 300);
    expect_eq(mc.sharePermille(.JsCode), 300);
    expect_eq(mc.sharePermille(.Assets), 200);
    expect_eq(mc.largestCategory() == MemoryCostCategory.JsCode, true);
});

test("memoryCost: merge and signed diff", () => {
    const a = MemoryCostBreakdown{
        incremental_graph_client: 100,
        js_code: 500,
        assets: 400,
    };
    const b = MemoryCostBreakdown{
        incremental_graph_server: 200,
        source_maps: 300,
        other: 100,
    };

    const merged = a.merge(b);
    expect_eq(merged.totalBytes(), 1600);
    expect_eq(merged.diffSigned(a), 600);
    expect_eq(a.diffSigned(merged), -600);
    expect_eq(a.diffSigned(a), 0);
});

test("memoryCost: RSS vs tracked leak diagnostic verdicts", () => {
    const t0 = MemoryCostBreakdown{ js_code: 10_000, source_maps: 5_000 };

    // 1. Healthy: both tracked and RSS below threshold
    const t1 = MemoryCostBreakdown{ js_code: 10_500, source_maps: 5_000 };
    const v1 = t1.diagnoseVsRss(t0, 100_000, 102_000, 10_000);
    expect_eq(v1 == MemoryLeakVerdict.Healthy, true);

    // 2. SuspectedUntrackedLeak: tracked flat, RSS jumped by +50_000 (>= 10_000)
    const v2 = t1.diagnoseVsRss(t0, 100_000, 150_000, 10_000);
    expect_eq(v2 == MemoryLeakVerdict.SuspectedUntrackedLeak, true);

    // 3. UnboundedTrackedGrowth: tracked itself grew by +20_000 (>= 10_000)
    const t2 = MemoryCostBreakdown{ js_code: 30_000, source_maps: 5_000 };
    const v3 = t2.diagnoseVsRss(t0, 100_000, 130_000, 10_000);
    expect_eq(v3 == MemoryLeakVerdict.UnboundedTrackedGrowth, true);
});

test("memoryCost: largestCategory tie-breaking across all buckets", () => {
    const m_assets = MemoryCostBreakdown{ assets: 999, js_code: 100 };
    expect_eq(m_assets.largestCategory() == MemoryCostCategory.Assets, true);

    const m_maps = MemoryCostBreakdown{ source_maps: 500, other: 10 };
    expect_eq(m_maps.largestCategory() == MemoryCostCategory.SourceMaps, true);

    const m_other = MemoryCostBreakdown{ other: 777 };
    expect_eq(m_other.largestCategory() == MemoryCostCategory.Other, true);
});

test("memoryCost: formatSummary output & BufferTooSmall guard", () => {
    const mc = MemoryCostBreakdown{
        incremental_graph_client: 100,
        incremental_graph_server: 200,
        js_code: 300,
        source_maps: 150,
        assets: 200,
        other: 50,
    };
    let buf: [128]u8 = [0; 128];
    const n = mc.formatSummary(buf[0..]) catch 0;
    expect_eq(
        strEq(buf[0..n], "total=1000B client=100 server=200 code=300 maps=150 assets=200 other=50"),
        true,
    );

    let tiny: [16]u8 = [0; 16];
    let hit_err = false;
    * = mc.formatSummary(tiny[0..]) catch |e| {
        hit_err = (e == MemoryCostError.BufferTooSmall);
        0
    };
    expect_eq(hit_err, true);
});
