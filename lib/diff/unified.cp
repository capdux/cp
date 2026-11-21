// Source: Absorbed from /data/dev/bun/src/test_runner/diff/printDiff.cp | Synced: 2026-11-21T17:06:09Z

/// 行级差异操作类型
pub enum DiffOpKind: u8 {
    equal = 0,
    delete = 1,
    insert = 2,

    fn prefix(): u8 {
        return switch (_) {
            .equal => ' ',
            .delete => '-',
            .insert => '+',
        };
    }
}

/// 单行差异记录（1-based 行号，insert 时 old_line=0，delete 时 new_line=0）
pub struct DiffLine {
    op: DiffOpKind = .equal,
    old_line: usize = 0,
    new_line: usize = 0,
    text: []const u8 = "",
}

/// 差异统计摘要
pub struct DiffSummary {
    added: usize = 0,
    removed: usize = 0,
    unchanged: usize = 0,

    fn isIdentical(): bool {
        return _.added == 0 && _.removed == 0;
    }
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn splitLines(text: []const u8, out: [][]const u8): usize {
    if (text.len == 0) { return 0; }
    let count: usize = 0;
    let i: usize = 0;
    while (i < text.len && count < out.len) {
        const start = i;
        while (i < text.len && text[i] != '\n') {
            i += 1;
        }
        let end = i;
        if (end > start && text[end - 1] == '\r') {
            end -= 1;
        }
        out[count] = text[start..end];
        count += 1;
        if (i < text.len && text[i] == '\n') {
            i += 1;
        }
    }
    return count;
}

/// 基于 LCS 动态规划计算行级最小编辑脚本（零堆分配，单侧最多 64 行）
/// 写入 `out_ops` 并返回实际写入的 `DiffLine` 数量
pub fn diffLines(old_text: []const u8, new_text: []const u8, out_ops: []DiffLine): usize {
    let old_lines: [64][]const u8 = [""; 64];
    let new_lines: [64][]const u8 = [""; 64];
    const n = splitLines(old_text, old_lines[0..]);
    const m = splitLines(new_text, new_lines[0..]);

    // 65 x 65 扁平 LCS DP 表（栈分配，零堆分配）
    let dp: [4225]u16 = [0; 4225];
    let i: usize = n;
    while (i > 0) {
        i -= 1;
        let j: usize = m;
        while (j > 0) {
            j -= 1;
            if (eqlBytes(old_lines[i], new_lines[j])) {
                dp[i * 65 + j] = dp[(i + 1) * 65 + (j + 1)] + 1;
            } else {
                const down = dp[(i + 1) * 65 + j];
                const right = dp[i * 65 + (j + 1)];
                dp[i * 65 + j] = if (down >= right) down else right;
            }
        }
    }

    // 前向回溯输出最小编辑脚本（平局时优先 delete 再 insert，符合标准 diff -u 惯例）
    let count: usize = 0;
    let oi: usize = 0;
    let ni: usize = 0;
    while ((oi < n || ni < m) && count < out_ops.len) {
        if (oi < n && ni < m && eqlBytes(old_lines[oi], new_lines[ni])) {
            out_ops[count] = DiffLine{
                op: .equal,
                old_line: oi + 1,
                new_line: ni + 1,
                text: old_lines[oi],
            };
            count += 1;
            oi += 1;
            ni += 1;
        } else if (oi < n && (ni == m || dp[(oi + 1) * 65 + ni] >= dp[oi * 65 + (ni + 1)])) {
            out_ops[count] = DiffLine{
                op: .delete,
                old_line: oi + 1,
                new_line: 0,
                text: old_lines[oi],
            };
            count += 1;
            oi += 1;
        } else {
            out_ops[count] = DiffLine{
                op: .insert,
                old_line: 0,
                new_line: ni + 1,
                text: new_lines[ni],
            };
            count += 1;
            ni += 1;
        }
    }

    return count;
}

/// 统计差异脚本中的新增、删除与未变更行数
pub fn summarize(ops: []const DiffLine): DiffSummary {
    let s = DiffSummary{};
    let i: usize = 0;
    while (i < ops.len) {
        switch (ops[i].op) {
            .equal => s.unchanged += 1,
            .delete => s.removed += 1,
            .insert => s.added += 1,
        }
        i += 1;
    }
    return s;
}

struct OutWriter {
    buf: []u8,
    pos: usize = 0,
    overflow: bool = false,

    fn writeByte(*_, c: u8) {
        if (_.pos < _.buf.len) {
            _.buf[_.pos] = c;
            _.pos += 1;
        } else {
            _.overflow = true;
        }
    }

    fn writeStr(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            _.writeByte(s[i]);
            i += 1;
        }
    }

    fn writeUsize(*_, val: usize) {
        let digits: [24]u8 = [0; 24];
        let dlen: usize = 0;
        let tmp = val;
        if (tmp == 0) {
            _.writeByte('0');
            return;
        }
        while (tmp > 0 && dlen < 24) {
            digits[dlen] = @intCast((tmp % 10) + '0');
            tmp /= 10;
            dlen += 1;
        }
        let k = dlen;
        while (k > 0) {
            k -= 1;
            _.writeByte(digits[k]);
        }
    }

    fn finish(): ([]const u8)? {
        if (_.overflow) { return null; }
        return _.buf[0.._.pos];
    }
}

fn findNextChange(ops: []const DiffLine, start_idx: usize): usize? {
    let i = start_idx;
    while (i < ops.len) {
        if (ops[i].op != .equal) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// 格式化为标准 Unified Diff（`--- old\n+++ new\n@@ -l1,c1 +l2,c2 @@\n`），自动合并相邻上下文 Hunk
/// 若两段文本完全相同，返回空切片；缓冲区不足返回 `null`
pub fn formatUnified(
    old_name: []const u8,
    new_name: []const u8,
    ops: []const DiffLine,
    context_lines: usize,
    out: []u8,
): ([]const u8)? {
    if (summarize(ops).isIdentical()) {
        return out[0..0];
    }

    let w = OutWriter{ buf: out };
    w.writeStr("--- ");
    w.writeStr(old_name);
    w.writeByte('\n');
    w.writeStr("+++ ");
    w.writeStr(new_name);
    w.writeByte('\n');

    let next_chg_opt = findNextChange(ops, 0);
    while (first_chg := next_chg_opt) {
        const hunk_start = if (first_chg > context_lines) first_chg - context_lines else 0;
        let last_chg = first_chg;

        // 合并上下文重叠或相邻的变更块
        while (true) {
            if (nc := findNextChange(ops, last_chg + 1)) {
                if (nc <= last_chg + (2 * context_lines) + 1) {
                    last_chg = nc;
                    continue;
                }
            }
            break;
        }

        const raw_end = last_chg + context_lines + 1;
        const hunk_end = if (raw_end < ops.len) raw_end else ops.len;

        let old_count: usize = 0;
        let new_count: usize = 0;
        let old_start: usize = 0;
        let new_start: usize = 0;

        let k = hunk_start;
        while (k < hunk_end) {
            const item = ops[k];
            if (item.op == .equal || item.op == .delete) {
                if (old_count == 0) { old_start = item.old_line; }
                old_count += 1;
            }
            if (item.op == .equal || item.op == .insert) {
                if (new_count == 0) { new_start = item.new_line; }
                new_count += 1;
            }
            k += 1;
        }

        // 若某侧在该 Hunk 中行数为 0，按标准 diff -u 规则取 Hunk 之前最后出现行号（空文件则为 0）
        if (old_count == 0) {
            let p = hunk_start;
            while (p > 0) {
                p -= 1;
                if (ops[p].old_line > 0) {
                    old_start = ops[p].old_line;
                    break;
                }
            }
        }
        if (new_count == 0) {
            let p = hunk_start;
            while (p > 0) {
                p -= 1;
                if (ops[p].new_line > 0) {
                    new_start = ops[p].new_line;
                    break;
                }
            }
        }

        w.writeStr("@@ -");
        w.writeUsize(old_start);
        w.writeByte(',');
        w.writeUsize(old_count);
        w.writeStr(" +");
        w.writeUsize(new_start);
        w.writeByte(',');
        w.writeUsize(new_count);
        w.writeStr(" @@\n");

        let m = hunk_start;
        while (m < hunk_end) {
            w.writeByte(ops[m].op.prefix());
            w.writeStr(ops[m].text);
            w.writeByte('\n');
            m += 1;
        }

        next_chg_opt = findNextChange(ops, hunk_end);
    }

    return w.finish();
}
