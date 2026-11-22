// Source: Absorbed from /data/dev/bun/src/install_types/SlicedString.cp | Synced: 2026-11-22T19:27:55Z
// Pure-CP zero-allocation buffer-backed SlicedString & compact SlicedSpan
// (safe range-checked sub-slicing, trimming, splitting, and compact offset/len export).

/// `SlicedString` 范围校验与切分错误集
pub err SlicedStringError {
    OutOfBounds,
    InvalidRange,
}

/// 8 字节紧凑子串跨度句柄（对齐 Bun `Semver.String` / lockfile 字符串池偏移+长度编码）
pub struct SlicedSpan {
    offset: u32 = 0,
    len: u32 = 0,

    fn end(): usize {
        return @as(usize, _.offset) + @as(usize, _.len);
    }

    fn isEmpty(): bool {
        return _.len == 0;
    }

    /// 从宿主缓冲区零拷贝还原切片（带越界保护）
    fn resolve(buf: []const u8): ([]const u8)! {
        const start = @as(usize, _.offset);
        const stop = start + @as(usize, _.len);
        if (stop < start || stop > buf.len) {
            return err.OutOfBounds;
        }
        return buf[start..stop];
    }
}

/// 零拷贝双段拆分结果（左右视图均保留原始宿主缓冲区与精确偏移）
pub struct SlicedSplit {
    left: SlicedString,
    right: SlicedString,
}

fn isAsciiSpace(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\n' || b == '\r';
}

fn eqSliceBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 绑定宿主缓冲区的零拷贝子串视图（对齐 Bun `install_types/SlicedString.cp`）
///
/// 通过 `(buf, offset, len)` 显式维护子切片与原始宿主缓冲区的归属关系，
/// 无需任何堆分配或裸指针运算即可完成安全二次切片、裁剪、拆分与紧凑 `SlicedSpan` 导出。
pub struct SlicedString {
    buf: []const u8,
    offset: usize = 0,
    len: usize = 0,

    /// 包装整个宿主缓冲区
    static fn fromBuffer(buf: []const u8): SlicedString {
        return SlicedString{
            buf: buf,
            offset: 0,
            len: buf.len,
        };
    }

    /// 按 `[start, end)` 半开区间构造绑定宿主缓冲区的子串视图
    static fn fromRange(buf: []const u8, start: usize, end: usize): SlicedString! {
        if (start > end) {
            return err.InvalidRange;
        }
        if (end > buf.len) {
            return err.OutOfBounds;
        }
        return SlicedString{
            buf: buf,
            offset: start,
            len: end - start,
        };
    }

    /// 从紧凑 `SlicedSpan` 还原绑定视图
    static fn fromSpan(buf: []const u8, span: SlicedSpan): SlicedString! {
        const start = @as(usize, span.offset);
        const stop = start + @as(usize, span.len);
        if (stop < start || stop > buf.len) {
            return err.OutOfBounds;
        }
        return SlicedString{
            buf: buf,
            offset: start,
            len: @as(usize, span.len),
        };
    }

    /// 返回当前视图对应的零拷贝字节切片
    fn slice(): []const u8 {
        return _.buf[_.offset..(_.offset + _.len)];
    }

    /// 返回原始宿主缓冲区
    fn buffer(): []const u8 {
        return _.buf;
    }

    /// 返回当前视图在宿主缓冲区中的字节起始偏移
    fn offsetInBuffer(): usize {
        return _.offset;
    }

    /// 返回当前视图字节长度
    fn length(): usize {
        return _.len;
    }

    fn isEmpty(): bool {
        return _.len == 0;
    }

    /// 导出紧凑 `(offset, len)` 跨度句柄（对齐 Bun `SlicedString.value()`）
    fn toSpan(): SlicedSpan {
        return SlicedSpan{
            offset: @as(u32, @intCast(_.offset)),
            len: @as(u32, @intCast(_.len)),
        };
    }

    /// 在当前视图内安全二次切片（`[start, end)` 为相对当前视图的偏移，对齐 Bun `SlicedString.sub`）
    fn subRange(start: usize, end: usize): SlicedString! {
        if (start > end) {
            return err.InvalidRange;
        }
        if (end > _.len) {
            return err.OutOfBounds;
        }
        return SlicedString{
            buf: _.buf,
            offset: _.offset + start,
            len: end - start,
        };
    }

    /// 零拷贝裁剪首尾 ASCII 空白字符
    fn trimAscii(): SlicedString {
        const s = _.slice();
        let start: usize = 0;
        let end: usize = s.len;
        while (start < end && isAsciiSpace(s[start])) {
            start += 1;
        }
        while (end > start && isAsciiSpace(s[end - 1])) {
            end -= 1;
        }
        return SlicedString{
            buf: _.buf,
            offset: _.offset + start,
            len: end - start,
        };
    }

    /// 若当前视图以 `prefix` 开头，返回剥离前缀后的新 `SlicedString`
    fn stripPrefix(prefix: []const u8): SlicedString? {
        const s = _.slice();
        if (s.len < prefix.len) {
            return null;
        }
        if (!eqSliceBytes(s[0..prefix.len], prefix)) {
            return null;
        }
        return SlicedString{
            buf: _.buf,
            offset: _.offset + prefix.len,
            len: _.len - prefix.len,
        };
    }

    /// 若当前视图以 `suffix` 结尾，返回剥离后缀后的新 `SlicedString`
    fn stripSuffix(suffix: []const u8): SlicedString? {
        const s = _.slice();
        if (s.len < suffix.len) {
            return null;
        }
        const tail_start = s.len - suffix.len;
        if (!eqSliceBytes(s[tail_start..s.len], suffix)) {
            return null;
        }
        return SlicedString{
            buf: _.buf,
            offset: _.offset,
            len: tail_start,
        };
    }

    /// 按首个匹配字节 `delim` 拆分为左右两段视图（均保持宿主缓冲区归属）
    fn splitOnceByte(delim: u8): SlicedSplit? {
        const s = _.slice();
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] == delim) {
                const left = SlicedString{
                    buf: _.buf,
                    offset: _.offset,
                    len: i,
                };
                const right = SlicedString{
                    buf: _.buf,
                    offset: _.offset + i + 1,
                    len: _.len - i - 1,
                };
                return SlicedSplit{ left: left, right: right };
            }
            i += 1;
        }
        return null;
    }

    /// 内容相等判定
    fn eql(other: []const u8): bool {
        return eqSliceBytes(_.slice(), other);
    }

    /// 计算当前子切片的 64-bit FNV-1a 哈希摘要（对齐 Bun `SlicedString.external()` 哈希能力）
    fn hashFnv64(): u64 {
        const s = _.slice();
        let h: u64 = 0xcbf29ce484222325;
        let i: usize = 0;
        while (i < s.len) {
            h = h ^ @as(u64, s[i]);
            h = h *% 0x100000001b3;
            i += 1;
        }
        return h;
    }
}
