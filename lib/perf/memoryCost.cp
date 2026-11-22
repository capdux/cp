// Source: Absorbed from /data/dev/bun/src/bake/DevServer/memory_cost.cp | Synced: 2026-11-22T15:37:29Z
// Zero-allocation structured memory-cost accounting & RSS vs tracked-memory leak diagnostics.

/// Error set for `MemoryCostBreakdown` formatting.
pub err MemoryCostError {
    BufferTooSmall,
}

/// Accounting bucket categories (exact parity with Bun `DevServer/memory_cost.cp`).
pub enum MemoryCostCategory: u8 {
    IncrementalGraphClient = 0,
    IncrementalGraphServer = 1,
    JsCode = 2,
    SourceMaps = 3,
    Assets = 4,
    Other = 5,
}

/// Diagnostic verdict comparing explicitly tracked memory growth against process RSS growth.
pub enum MemoryLeakVerdict: u8 {
    /// Both tracked memory and RSS delta are within acceptable threshold.
    Healthy = 0,
    /// Tracked memory remained stable while RSS grew significantly (likely untracked leak).
    SuspectedUntrackedLeak = 1,
    /// Explicitly tracked memory itself grew beyond threshold (likely missing GC / eviction).
    UnboundedTrackedGrowth = 2,
}

/// Estimate bytes occupied by a contiguous slice (`len * elem_size`, saturating on overflow).
pub fn estimateSliceBytes(len: usize, elem_size: usize): usize {
    const prod = u128(len) * u128(elem_size);
    const max_u: u128 = u128(@as(usize, @bitCast(@as(isize, -1))));
    if (prod > max_u) return usize(max_u);
    return usize(prod);
}

/// Estimate bytes reserved by a dynamic container's backing capacity (`capacity * elem_size`).
pub fn estimateCapacityBytes(capacity: usize, elem_size: usize): usize {
    return estimateSliceBytes(capacity, elem_size);
}

fn writeStr(out: []u8, pos: *usize, s: []const u8): usize ! MemoryCostError {
    if (pos.* + s.len > out.len) return err.BufferTooSmall;
    for (i of 0..s.len) {
        out[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return pos.*;
}

fn writeUsize(out: []u8, pos: *usize, val: usize): usize ! MemoryCostError {
    let tmp: [24]u8 = [0; 24];
    let len: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = '0';
        len = 1;
    } else {
        while (v > 0) {
            tmp[len] = u8('0') + u8(v % 10);
            v = v / 10;
            len += 1;
        }
    }
    if (pos.* + len > out.len) return err.BufferTooSmall;
    for (i of 0..len) {
        out[pos.* + i] = tmp[len - 1 - i];
    }
    pos.* += len;
    return pos.*;
}

/// Detailed byte accounting breakdown for long-lived server / bundler / runtime processes.
/// Mirrors Bun `DevServer/memory_cost.cp` `MemoryCostStruct`.
pub struct MemoryCostBreakdown {
    incremental_graph_client: usize = 0,
    incremental_graph_server: usize = 0,
    js_code: usize = 0,
    source_maps: usize = 0,
    assets: usize = 0,
    other: usize = 0,

    /// Sum of all 6 explicitly tracked memory categories in bytes.
    fn totalBytes(): usize {
        return _.incremental_graph_client +
            _.incremental_graph_server +
            _.js_code +
            _.source_maps +
            _.assets +
            _.other;
    }

    /// Add `bytes` to the specified category.
    fn add(*_, cat: MemoryCostCategory, bytes: usize) {
        switch (cat) {
            .IncrementalGraphClient => _.incremental_graph_client += bytes,
            .IncrementalGraphServer => _.incremental_graph_server += bytes,
            .JsCode => _.js_code += bytes,
            .SourceMaps => _.source_maps += bytes,
            .Assets => _.assets += bytes,
            .Other => _.other += bytes,
        }
    }

    /// Read byte count for a single category.
    fn bytesFor(cat: MemoryCostCategory): usize {
        return switch (cat) {
            .IncrementalGraphClient => _.incremental_graph_client,
            .IncrementalGraphServer => _.incremental_graph_server,
            .JsCode => _.js_code,
            .SourceMaps => _.source_maps,
            .Assets => _.assets,
            .Other => _.other,
        };
    }

    /// Return a category's share of total tracked memory in permille (`0..=1000`).
    fn sharePermille(cat: MemoryCostCategory): u32 {
        const tot = _.totalBytes();
        if (tot == 0) return 0;
        return u32((u128(_.bytesFor(cat)) * 1000) / u128(tot));
    }

    /// Identify the category holding the largest byte footprint.
    fn largestCategory(): MemoryCostCategory {
        let best_cat = MemoryCostCategory.IncrementalGraphClient;
        let best_val = _.incremental_graph_client;

        if (_.incremental_graph_server > best_val) {
            best_val = _.incremental_graph_server;
            best_cat = .IncrementalGraphServer;
        }
        if (_.js_code > best_val) {
            best_val = _.js_code;
            best_cat = .JsCode;
        }
        if (_.source_maps > best_val) {
            best_val = _.source_maps;
            best_cat = .SourceMaps;
        }
        if (_.assets > best_val) {
            best_val = _.assets;
            best_cat = .Assets;
        }
        if (_.other > best_val) {
            best_cat = .Other;
        }
        return best_cat;
    }

    /// Merge two breakdowns by summing each category.
    fn merge(other: MemoryCostBreakdown): MemoryCostBreakdown {
        return MemoryCostBreakdown{
            incremental_graph_client: _.incremental_graph_client + other.incremental_graph_client,
            incremental_graph_server: _.incremental_graph_server + other.incremental_graph_server,
            js_code: _.js_code + other.js_code,
            source_maps: _.source_maps + other.source_maps,
            assets: _.assets + other.assets,
            other: _.other + other.other,
        };
    }

    /// Signed byte delta `self.totalBytes() - earlier.totalBytes()`.
    fn diffSigned(earlier: MemoryCostBreakdown): i64 {
        const cur = _.totalBytes();
        const prev = earlier.totalBytes();
        if (cur >= prev) {
            return i64(cur - prev);
        }
        const delta: i64 = i64(prev - cur);
        return -delta;
    }

    /// Diagnose memory health by comparing tracked growth against RSS growth (Bun `memory_cost.cp` heuristic):
    /// - If tracked grew by >= `growth_threshold_bytes`: `.UnboundedTrackedGrowth`
    /// - Else if RSS grew by >= `growth_threshold_bytes` while tracked stayed below threshold: `.SuspectedUntrackedLeak`
    /// - Else: `.Healthy`.
    fn diagnoseVsRss(
        earlier: MemoryCostBreakdown,
        prev_rss_bytes: usize,
        cur_rss_bytes: usize,
        growth_threshold_bytes: usize,
    ): MemoryLeakVerdict {
        const cur_tracked = _.totalBytes();
        const prev_tracked = earlier.totalBytes();
        const tracked_growth: usize = if (cur_tracked > prev_tracked) (cur_tracked - prev_tracked) else 0;
        const rss_growth: usize = if (cur_rss_bytes > prev_rss_bytes) (cur_rss_bytes - prev_rss_bytes) else 0;

        if (tracked_growth >= growth_threshold_bytes) {
            return .UnboundedTrackedGrowth;
        }
        if (rss_growth >= growth_threshold_bytes) {
            return .SuspectedUntrackedLeak;
        }
        return .Healthy;
    }

    /// Format a compact zero-allocation summary string into `out`.
    /// Example: `"total=1000B client=100 server=200 code=300 maps=150 assets=200 other=50"`
    fn formatSummary(out: []u8): usize ! MemoryCostError {
        let pos: usize = 0;
        * = writeStr(out, *pos, "total=")?;
        * = writeUsize(out, *pos, _.totalBytes())?;
        * = writeStr(out, *pos, "B client=")?;
        * = writeUsize(out, *pos, _.incremental_graph_client)?;
        * = writeStr(out, *pos, " server=")?;
        * = writeUsize(out, *pos, _.incremental_graph_server)?;
        * = writeStr(out, *pos, " code=")?;
        * = writeUsize(out, *pos, _.js_code)?;
        * = writeStr(out, *pos, " maps=")?;
        * = writeUsize(out, *pos, _.source_maps)?;
        * = writeStr(out, *pos, " assets=")?;
        * = writeUsize(out, *pos, _.assets)?;
        * = writeStr(out, *pos, " other=")?;
        * = writeUsize(out, *pos, _.other)?;
        return pos;
    }
}
