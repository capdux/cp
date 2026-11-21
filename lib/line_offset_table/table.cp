// Source: Absorbed from /data/dev/bun/src/sourcemap/LineOffsetTable.cp

pub err Error {
    SourceTooLarge,
}

/// 0-based 行列坐标（提供 1-based 便捷换算用于编译器诊断展示）
pub struct LineCol {
    line: usize,
    column: usize,

    fn line1(): usize {
        return _.line + 1;
    }

    fn column1(): usize {
        return _.column + 1;
    }

    fn eql(other: LineCol): bool {
        return _.line == other.line && _.column == other.column;
    }
}

/// 单行字节偏移范围（区分不含换行符与含换行符结束偏移）
pub struct LineRange {
    start: usize,
    end: usize,
    end_with_newline: usize,
}

/// 基于二分查找的源码行列偏移表（支持 LF/CRLF/CR 与 `~init()` 确定性 RAII 释放）
pub struct LineOffsetTable {
    allocator: std.mem.Allocator,
    line_starts: []const u32,
    source_len: usize,

    ~init() {
        _.allocator.free(_.line_starts);
    }

    /// 单遍扫描源码构建行起始偏移表（支持 `\n`、`\r\n` 与 `\r`）
    static fn build(allocator: std.mem.Allocator, source: string): LineOffsetTable! {
        if (source.len > std.math.maxInt(u32)) {
            return err.SourceTooLarge;
        }

        let starts = std.array_list.Managed(u32).init(allocator);
        defer starts.deinit();

        starts.append(0)?;

        let i: usize = 0;
        while (i < source.len) {
            const ch = source[i];
            if (ch == '\n') {
                starts.append((i + 1) as u32)?;
                i += 1;
            } else if (ch == '\r') {
                if (i + 1 < source.len && source[i + 1] == '\n') {
                    starts.append((i + 2) as u32)?;
                    i += 2;
                } else {
                    starts.append((i + 1) as u32)?;
                    i += 1;
                }
            } else {
                i += 1;
            }
        }

        const owned = starts.toOwnedSlice()?;
        return LineOffsetTable{
            allocator: allocator,
            line_starts: owned,
            source_len: source.len,
        };
    }

    /// 返回总行数（空源码至少为 1 行）
    fn lineCount(): usize {
        return _.line_starts.len;
    }

    /// 返回建表时的源码总字节数
    fn sourceLen(): usize {
        return _.source_len;
    }

    /// 返回第 line 行（0-based）的起始字节偏移；越界返回 null
    fn lineStart(line: usize): usize? {
        if (line >= _.line_starts.len) {
            return null;
        }
        return _.line_starts[line] as usize;
    }

    /// O(log N) 二分查找字节偏移所在行号（0-based）
    fn findLine(offset: usize): usize {
        let lo: usize = 0;
        let count: usize = _.line_starts.len;
        while (count > 0) {
            const step = count / 2;
            const mid = lo + step;
            if ((_.line_starts[mid] as usize) <= offset) {
                lo = mid + 1;
                count = count - step - 1;
            } else {
                count = step;
            }
        }
        if (lo == 0) {
            return 0;
        }
        return lo - 1;
    }

    /// 字节偏移 -> (line, column) 双向定位；超过 source_len 返回 null
    fn offsetToLineCol(offset: usize): LineCol? {
        if (offset > _.source_len) {
            return null;
        }
        const line = _.findLine(offset);
        const start = _.line_starts[line] as usize;
        return LineCol{
            line: line,
            column: offset - start,
        };
    }

    /// (line, column) -> 字节偏移反向换算；行越界或列越出本行范围返回 null
    fn lineColToOffset(line: usize, column: usize): usize? {
        if (line >= _.line_starts.len) {
            return null;
        }
        const start = _.line_starts[line] as usize;
        const next_start: usize = if (line + 1 < _.line_starts.len)
            (_.line_starts[line + 1] as usize)
        else
            _.source_len;

        const target = start + column;
        if (line + 1 < _.line_starts.len) {
            if (target >= next_start) {
                return null;
            }
        } else {
            if (target > _.source_len) {
                return null;
            }
        }
        return target;
    }

    /// 获取第 line 行的字节范围（自动剥离尾部 `\r\n` / `\n` / `\r`）
    fn lineRange(source: string, line: usize): LineRange? {
        if (line >= _.line_starts.len) {
            return null;
        }
        const start = _.line_starts[line] as usize;
        const end_nl: usize = if (line + 1 < _.line_starts.len)
            (_.line_starts[line + 1] as usize)
        else
            _.source_len;

        if (end_nl > source.len || start > end_nl) {
            return null;
        }

        let content_end = end_nl;
        if (content_end > start && source[content_end - 1] == '\n') {
            content_end -= 1;
            if (content_end > start && source[content_end - 1] == '\r') {
                content_end -= 1;
            }
        } else if (content_end > start && source[content_end - 1] == '\r') {
            content_end -= 1;
        }

        return LineRange{
            start: start,
            end: content_end,
            end_with_newline: end_nl,
        };
    }

    /// 提取第 line 行的纯内容切片（不含尾部换行符）；越界返回 null
    fn lineSlice(source: string, line: usize): string? {
        if (r := _.lineRange(source, line)) {
            return source[r.start..r.end];
        }
        return null;
    }
}
