// Source: Absorbed from /data/dev/bun/src/cli/test/parallel/aggregate.cp | Synced: 2026-11-22T05:05:00Z
// ============================================================================
// Parallel Test Worker Aggregator: JUnit XML & LCOV Coverage Fragment Merging
// Absorbed from Bun: src/cli/test/parallel/aggregate.cp
// ============================================================================

/// 并行 JUnit XML 合并汇总统计（保证外层 `<testsuites>` 总数与内层 `<testsuite>` 之和严格一致）
pub struct JUnitTotals {
    tests: u32 = 0,
    failures: u32 = 0,
    skipped: u32 = 0,
}

/// JUnit XML 片段合并产物（RAII 自动释放底层缓冲）
pub struct JUnitMergeResult {
    xml: []u8,
    totals: JUnitTotals,
    allocator: std.mem.Allocator,

    ~init() {
        if (_.xml.len > 0) {
            _.allocator.free(_.xml);
            _.xml = _.xml[0..0];
        }
    }

    fn deinit(*_) {
        if (_.xml.len > 0) {
            _.allocator.free(_.xml);
            _.xml = _.xml[0..0];
        }
    }
}

/// 覆盖率阈值配置（取值范围 `[0.0, 1.0]`）
pub struct CoverageThreshold {
    functions: f64 = 0.0,
    lines: f64 = 0.0,
}

/// 单文件合并后的覆盖率记录（LCOV `SF` / `FNF` / `FNH` / `DA` 饱和累加）
pub struct MergedCoverageFile {
    path: []u8,
    fnf: u32 = 0,
    fnh: u32 = 0,
    da: std.AutoArrayHashMapUnmanaged(u32, u32) = .{},

    /// 返回命中行数（`LH`，统计 `hits > 0` 的行）
    fn lh(): u32 {
        let hit: u32 = 0;
        const vals = _.da.values();
        let i: usize = 0;
        while (i < vals.len) {
            if (vals[i] > 0) {
                hit += 1;
            }
            i += 1;
        }
        return hit;
    }

    /// 返回总可执行行数（`LF`）
    fn lf(): u32 {
        return @intCast(_.da.count());
    }

    /// 返回函数覆盖率（`fnf == 0` 时按 1.0 计，与 Bun 语义对齐）
    fn funcRatio(): f64 {
        if (_.fnf == 0) {
            return 1.0;
        }
        const h: f64 = @floatFromInt(_.fnh);
        const f: f64 = @floatFromInt(_.fnf);
        const r = h / f;
        return if (r > 1.0) 1.0 else r;
    }

    /// 返回行覆盖率（`lf == 0` 时按 1.0 计，与 Bun 语义对齐）
    fn lineRatio(): f64 {
        const total = _.lf();
        if (total == 0) {
            return 1.0;
        }
        const h: f64 = @floatFromInt(_.lh());
        const t: f64 = @floatFromInt(total);
        return h / t;
    }

    /// 将所有 `hits == 0` 的未覆盖行号升序合并为紧凑区间字符串（如 `"3,5-8,12"`）
    fn formatUncoveredRanges(allocator: std.mem.Allocator): []u8! {
        let out: std.ArrayListUnmanaged(u8) = .{};
        errdefer out.deinit(allocator);

        const keys = _.da.keys();
        if (keys.len == 0) {
            return out.toOwnedSlice(allocator);
        }

        let sorted = allocator.dupe(u32, keys)?;
        defer allocator.free(sorted);
        sortU32Asc(sorted);

        let first = true;
        let range_start: u32 = 0;
        let range_end: u32 = 0;

        let i: usize = 0;
        while (i < sorted.len) {
            const ln = sorted[i];
            i += 1;
            const hits = _.da.get(ln) ?? 0;
            if (hits != 0) {
                continue;
            }
            if (range_start == 0) {
                range_start = ln;
                range_end = ln;
            } else if (ln == range_end + 1) {
                range_end = ln;
            } else {
                appendLineRange(allocator, &out, &first, range_start, range_end)?;
                range_start = ln;
                range_end = ln;
            }
        }
        if (range_start != 0) {
            appendLineRange(allocator, &out, &first, range_start, range_end)?;
        }
        return out.toOwnedSlice(allocator);
    }
}

/// 多 Worker LCOV 覆盖率片段聚合器（确定性排序 + 饱和累加 + 阈值校验）
pub struct CoverageAggregator {
    allocator: std.mem.Allocator,
    files: std.StringArrayHashMapUnmanaged(MergedCoverageFile) = .{},

    static fn init(allocator: std.mem.Allocator): CoverageAggregator {
        return CoverageAggregator{
            allocator: allocator,
        };
    }

    ~init() {
        _.freeAll();
    }

    fn deinit(*_) {
        _.freeAll();
    }

    fn freeAll(*_) {
        const vals = _.files.values();
        let i: usize = 0;
        while (i < vals.len) {
            let entry = &vals[i];
            entry.da.deinit(_.allocator);
            _.allocator.free(entry.path);
            i += 1;
        }
        _.files.deinit(_.allocator);
        _.files = .{};
    }

    /// 吸收一段 LCOV 文本片段，将 `SF:`/`DA:`/`FNF:`/`FNH:` 增量合并到聚合表
    fn ingestLcov(*_, lcov_text: []const u8): ! {
        let cur: (*MergedCoverageFile)? = null;
        let it = std.mem.splitScalar(u8, lcov_text, '\n');

        while (raw := it.next()) {
            const line = std.mem.trimEnd(u8, raw, "\r");
            if (std.mem.startsWith(u8, line, "SF:")) {
                const name = line[3..];
                if (name.len == 0) {
                    cur = null;
                    continue;
                }
                let gop = _.files.getOrPut(_.allocator, name)?;
                if (!gop.found_existing) {
                    const owned_path = _.allocator.dupe(u8, name)?;
                    gop.key_ptr.* = owned_path;
                    gop.value_ptr.* = MergedCoverageFile{
                        path: owned_path,
                    };
                }
                cur = gop.value_ptr;
            } else if (std.mem.eql(u8, line, "end_of_record")) {
                cur = null;
            } else if (fc := cur) {
                if (std.mem.startsWith(u8, line, "DA:")) {
                    let parts = std.mem.splitScalar(u8, line[3..], ',');
                    const ln_str = parts.next() ?? continue;
                    const cnt_str = parts.next() ?? continue;
                    const ln = std.fmt.parseInt(u32, ln_str, 10) catch continue;
                    const cnt = std.fmt.parseInt(u32, cnt_str, 10) catch continue;
                    let da_gop = fc.da.getOrPut(_.allocator, ln)?;
                    if (da_gop.found_existing) {
                        da_gop.value_ptr.* = da_gop.value_ptr.* +| cnt;
                    } else {
                        da_gop.value_ptr.* = cnt;
                    }
                } else if (std.mem.startsWith(u8, line, "FNF:")) {
                    const v = std.fmt.parseInt(u32, line[4..], 10) catch 0;
                    if (v > fc.fnf) {
                        fc.fnf = v;
                    }
                } else if (std.mem.startsWith(u8, line, "FNH:")) {
                    const v = std.fmt.parseInt(u32, line[4..], 10) catch 0;
                    if (v > fc.fnh) {
                        fc.fnh = v;
                    }
                }
            }
        }
    }

    /// 返回聚合后的源文件总数
    fn fileCount(): usize {
        return _.files.count();
    }

    /// 按路径查找合并后的单文件覆盖率记录
    fn getFile(path: []const u8): (*const MergedCoverageFile)? {
        if (ptr := _.files.getPtr(path)) {
            return ptr;
        }
        return null;
    }

    /// 评估是否所有文件均满足覆盖率阈值（空集视为通过）
    fn evaluateThresholds(threshold: CoverageThreshold): bool {
        const vals = _.files.values();
        let i: usize = 0;
        while (i < vals.len) {
            const fc = &vals[i];
            if (fc.funcRatio() < threshold.functions || fc.lineRatio() < threshold.lines) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// 计算所有文件的平均行覆盖率（空集返回 1.0）
    fn averageLineRatio(): f64 {
        const vals = _.files.values();
        if (vals.len == 0) {
            return 1.0;
        }
        let sum: f64 = 0.0;
        let i: usize = 0;
        while (i < vals.len) {
            sum += vals[i].lineRatio();
            i += 1;
        }
        const n: f64 = @floatFromInt(vals.len);
        return sum / n;
    }

    /// 计算所有文件的平均函数覆盖率（空集返回 1.0）
    fn averageFuncRatio(): f64 {
        const vals = _.files.values();
        if (vals.len == 0) {
            return 1.0;
        }
        let sum: f64 = 0.0;
        let i: usize = 0;
        while (i < vals.len) {
            sum += vals[i].funcRatio();
            i += 1;
        }
        const n: f64 = @floatFromInt(vals.len);
        return sum / n;
    }

    /// 按文件路径字典序与行号升序，渲染标准合并后的 `lcov.info` 文本
    fn renderMergedLcov(out_alloc: std.mem.Allocator): []u8! {
        let out: std.ArrayListUnmanaged(u8) = .{};
        errdefer out.deinit(out_alloc);

        const count = _.files.count();
        if (count == 0) {
            return out.toOwnedSlice(out_alloc);
        }

        // 复制索引数组并按路径字典序排序，确保跨平台输出稳定
        let order = out_alloc.alloc(usize, count)?;
        defer out_alloc.free(order);
        let k: usize = 0;
        while (k < count) {
            order[k] = k;
            k += 1;
        }
        const keys = _.files.keys();
        sortIndicesByKey(order, keys);

        const vals = _.files.values();
        let idx: usize = 0;
        while (idx < count) {
            const fc = &vals[order[idx]];
            idx += 1;

            out.appendSlice(out_alloc, "TN:\nSF:")?;
            out.appendSlice(out_alloc, fc.path)?;
            out.appendSlice(out_alloc, "\nFNF:")?;
            appendU32(out_alloc, &out, fc.fnf)?;
            out.appendSlice(out_alloc, "\nFNH:")?;
            appendU32(out_alloc, &out, fc.fnh)?;
            out.append(out_alloc, '\n')?;

            const da_keys = fc.da.keys();
            let sorted_lines = out_alloc.dupe(u32, da_keys)?;
            defer out_alloc.free(sorted_lines);
            sortU32Asc(sorted_lines);

            let j: usize = 0;
            while (j < sorted_lines.len) {
                const ln = sorted_lines[j];
                const hits = fc.da.get(ln) ?? 0;
                out.appendSlice(out_alloc, "DA:")?;
                appendU32(out_alloc, &out, ln)?;
                out.append(out_alloc, ',')?;
                appendU32(out_alloc, &out, hits)?;
                out.append(out_alloc, '\n')?;
                j += 1;
            }

            out.appendSlice(out_alloc, "LF:")?;
            appendU32(out_alloc, &out, fc.lf())?;
            out.appendSlice(out_alloc, "\nLH:")?;
            appendU32(out_alloc, &out, fc.lh())?;
            out.appendSlice(out_alloc, "\nend_of_record\n")?;
        }

        return out.toOwnedSlice(out_alloc);
    }
}

/// 从 `<testsuites ...>` 标签头部提取指定无符号整数属性（如 `tests` / `failures` / `skipped`）
pub fn parseJUnitAttrU32(head: []const u8, attr_name: []const u8): u32 {
    if (attr_name.len == 0 || head.len < attr_name.len + 4) {
        return 0;
    }
    let pos: usize = 0;
    while (pos < head.len) {
        if (head[pos] == ' ') {
            const name_start = pos + 1;
            const eq_pos = name_start + attr_name.len;
            if (eq_pos + 2 <= head.len &&
                std.mem.eql(u8, head[name_start..eq_pos], attr_name) &&
                head[eq_pos] == '=' &&
                head[eq_pos + 1] == '"')
            {
                const val_start = eq_pos + 2;
                let val_end = val_start;
                while (val_end < head.len && head[val_end] != '"') {
                    val_end += 1;
                }
                if (val_end < head.len && val_end > val_start) {
                    return std.fmt.parseInt(u32, head[val_start..val_end], 10) catch 0;
                }
                return 0;
            }
        }
        pos += 1;
    }
    return 0;
}

/// 将字符串按 XML 1.0 实体规范转义并追加到目标缓冲区
pub fn appendXmlEscaped(
    allocator: std.mem.Allocator,
    out: *std.ArrayListUnmanaged(u8),
    text: []const u8,
): ! {
    let i: usize = 0;
    while (i < text.len) {
        const ch = text[i];
        i += 1;
        if (ch == '&') {
            out.appendSlice(allocator, "&amp;")?;
        } else if (ch == '<') {
            out.appendSlice(allocator, "&lt;")?;
        } else if (ch == '>') {
            out.appendSlice(allocator, "&gt;")?;
        } else if (ch == '"') {
            out.appendSlice(allocator, "&quot;")?;
        } else if (ch == '\'') {
            out.appendSlice(allocator, "&apos;")?;
        } else {
            out.append(allocator, ch)?;
        }
    }
}

/// 合并多个 Worker 的 JUnit XML 片段，并为异常终止的测试文件合成失败 `<testsuite>`
pub fn mergeJUnitFragments(
    allocator: std.mem.Allocator,
    fragments: []const []const u8,
    crashed_files: []const []const u8,
    assertions: u64,
    elapsed_seconds: f64,
): JUnitMergeResult! {
    let body: std.ArrayListUnmanaged(u8) = .{};
    defer body.deinit(allocator);

    let totals = JUnitTotals{};

    let i: usize = 0;
    while (i < fragments.len) {
        const frag = fragments[i];
        i += 1;

        const open_start = std.mem.indexOf(u8, frag, "<testsuites") ?? continue;
        const rel_head_end = std.mem.indexOfScalar(u8, frag[open_start..], '>') ?? continue;
        const head_end = open_start + rel_head_end;
        const head = frag[open_start..head_end];

        totals.tests = totals.tests +| parseJUnitAttrU32(head, "tests");
        totals.failures = totals.failures +| parseJUnitAttrU32(head, "failures");
        totals.skipped = totals.skipped +| parseJUnitAttrU32(head, "skipped");

        const body_start = head_end + 1;
        const body_end = std.mem.lastIndexOf(u8, frag, "</testsuites>") ?? continue;
        if (body_start >= body_end) {
            continue;
        }
        const inner = std.mem.trim(u8, frag[body_start..body_end], "\r\n");
        if (inner.len == 0) {
            continue;
        }
        body.appendSlice(allocator, inner)?;
        body.append(allocator, '\n')?;
    }

    let c: usize = 0;
    while (c < crashed_files.len) {
        const rel = crashed_files[c];
        c += 1;
        body.appendSlice(allocator, "  <testsuite name=\"")?;
        appendXmlEscaped(allocator, &body, rel)?;
        body.appendSlice(allocator, "\" tests=\"1\" assertions=\"0\" failures=\"1\" skipped=\"0\" time=\"0\">\n    <testcase name=\"(worker exited)\" classname=\"")?;
        appendXmlEscaped(allocator, &body, rel)?;
        body.appendSlice(allocator, "\">\n      <failure message=\"worker exited before reporting results\"></failure>\n    </testcase>\n  </testsuite>\n\n")?;
        totals.tests = totals.tests +| 1;
        totals.failures = totals.failures +| 1;
    }

    let contents: std.ArrayListUnmanaged(u8) = .{};
    errdefer contents.deinit(allocator);

    let num_buf: [64]u8 = [0; 64];
    contents.appendSlice(allocator, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<testsuites name=\"cap test\" tests=\"")?;
    appendU32(allocator, &contents, totals.tests)?;
    contents.appendSlice(allocator, "\" assertions=\"")?;
    const assert_str = std.fmt.bufPrint(num_buf[0..], "{d}", .{assertions}) catch "0";
    contents.appendSlice(allocator, assert_str)?;
    contents.appendSlice(allocator, "\" failures=\"")?;
    appendU32(allocator, &contents, totals.failures)?;
    contents.appendSlice(allocator, "\" skipped=\"")?;
    appendU32(allocator, &contents, totals.skipped)?;
    contents.appendSlice(allocator, "\" time=\"")?;
    const safe_elapsed = if (elapsed_seconds < 0.0) 0.0 else elapsed_seconds;
    const time_str = std.fmt.bufPrint(num_buf[0..], "{d:.3}", .{safe_elapsed}) catch "0.000";
    contents.appendSlice(allocator, time_str)?;
    contents.appendSlice(allocator, "\">\n\n")?;
    contents.appendSlice(allocator, body.items)?;
    contents.appendSlice(allocator, "</testsuites>\n")?;

    return JUnitMergeResult{
        xml: contents.toOwnedSlice(allocator)?,
        totals: totals,
        allocator: allocator,
    };
}

// ============================================================================
// 内部辅助函数
// ============================================================================

fn appendU32(allocator: std.mem.Allocator, out: *std.ArrayListUnmanaged(u8), v: u32): ! {
    let buf: [16]u8 = [0; 16];
    const s = std.fmt.bufPrint(buf[0..], "{d}", .{v}) catch "0";
    out.appendSlice(allocator, s)?;
}

fn appendLineRange(
    allocator: std.mem.Allocator,
    out: *std.ArrayListUnmanaged(u8),
    first: *bool,
    a: u32,
    b: u32,
): ! {
    if (first.*) {
        first.* = false;
    } else {
        out.append(allocator, ',')?;
    }
    appendU32(allocator, out, a)?;
    if (b > a) {
        out.append(allocator, '-')?;
        appendU32(allocator, out, b)?;
    }
}

fn sortU32Asc(items: []u32) {
    if (items.len <= 1) {
        return;
    }
    let i: usize = 1;
    while (i < items.len) {
        const key = items[i];
        let j = i;
        while (j > 0 && items[j - 1] > key) {
            items[j] = items[j - 1];
            j -= 1;
        }
        items[j] = key;
        i += 1;
    }
}

fn sortIndicesByKey(order: []usize, keys: []const []const u8) {
    if (order.len <= 1) {
        return;
    }
    let i: usize = 1;
    while (i < order.len) {
        const key_idx = order[i];
        let j = i;
        while (j > 0 && std.mem.lessThan(u8, keys[key_idx], keys[order[j - 1]])) {
            order[j] = order[j - 1];
            j -= 1;
        }
        order[j] = key_idx;
        i += 1;
    }
}
