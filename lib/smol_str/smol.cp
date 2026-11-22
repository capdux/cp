// Source: Absorbed from /data/dev/bun/src/string/SmolStr.cp | Synced: 2026-11-22T20:42:08Z

pub const INLINE_MAX_LEN: usize = 15;

pub err Error {
    StringTooLong,
}

/// 小字符串内联优化结构体（<= 15 字节零堆分配内联存储，> 15 字节透明转堆存储，含 `~init()` RAII 自动释放）
pub struct SmolStr {
    allocator: std.mem.Allocator? = null,
    inline_buf: [INLINE_MAX_LEN]u8 = [0; INLINE_MAX_LEN],
    inline_len: u8 = 0,
    heap_buf: ([]u8)? = null,
    heap_len: usize = 0,

    ~init() {
        if (buf := _.heap_buf) {
            if (alloc := _.allocator) {
                alloc.free(buf);
            }
            _.heap_buf = null;
            _.heap_len = 0;
        }
    }

    /// 构造零堆分配空字符串
    static fn empty(): SmolStr {
        return SmolStr{};
    }

    /// 构造零堆分配单字节内联字符串
    static fn fromChar(ch: u8): SmolStr {
        let s = SmolStr{};
        s.inline_buf[0] = ch;
        s.inline_len = 1;
        return s;
    }

    /// 纯内联构造（零堆分配）；若长度超过 INLINE_MAX_LEN 返回 err.StringTooLong
    static fn fromInline(src: string): SmolStr! {
        if (src.len > INLINE_MAX_LEN) {
            return err.StringTooLong;
        }
        let s = SmolStr{};
        if (src.len > 0) {
            @memcpy(s.inline_buf[0..src.len], src);
            s.inline_len = src.len as u8;
        }
        return s;
    }

    /// 从切片构造 SmolStr：<= 15 字节零堆分配，> 15 字节透明堆分配
    static fn fromSlice(allocator: std.mem.Allocator, src: string): SmolStr! {
        if (src.len <= INLINE_MAX_LEN) {
            let s = SmolStr{ allocator: allocator };
            if (src.len > 0) {
                @memcpy(s.inline_buf[0..src.len], src);
                s.inline_len = src.len as u8;
            }
            return s;
        }

        const buf = allocator.alloc(u8, src.len)?;
        @memcpy(buf[0..src.len], src);
        return SmolStr{
            allocator: allocator,
            heap_buf: buf,
            heap_len: src.len,
        };
    }

    /// 是否处于栈上内联存储模式（零堆分配）
    fn isInline(): bool {
        return _.heap_buf == null;
    }

    /// 是否处于堆存储模式
    fn isHeap(): bool {
        return _.heap_buf != null;
    }

    /// 是否为空字符串
    fn isEmpty(): bool {
        return _.len() == 0;
    }

    /// 返回当前字节长度
    fn len(): usize {
        if (_.heap_buf != null) {
            return _.heap_len;
        }
        return _.inline_len as usize;
    }

    /// 返回只读字符串切片视图
    fn slice(): string {
        if (buf := _.heap_buf) {
            return buf[0.._.heap_len];
        }
        return _.inline_buf[0 .. _.inline_len as usize];
    }

    /// 与另一个 SmolStr 按内容比较是否相等
    fn eql(other: SmolStr): bool {
        return std.mem.eql(u8, _.slice(), other.slice());
    }

    /// 与普通 string 切片按内容比较是否相等
    fn eqlStr(other: string): bool {
        return std.mem.eql(u8, _.slice(), other);
    }

    /// 计算内容 FNV-1a 64-bit 哈希（与存储形态无关）
    fn hash(): u64 {
        const bytes = _.slice();
        let h: u64 = 0xcbf29ce484222325;
        let i: usize = 0;
        while (i < bytes.len) {
            h ^= bytes[i] as u64;
            h *%= 0x100000001b3;
            i += 1;
        }
        return h;
    }

    /// 生成独立深拷贝（内联串零堆分配，堆串分配独立缓冲）
    fn clone(allocator: std.mem.Allocator): SmolStr! {
        return SmolStr.fromSlice(allocator, _.slice());
    }

    /// 追加单个字节；超过内联阈值时自动透明迁移到堆上
    fn appendChar(*_, allocator: std.mem.Allocator, ch: u8)! {
        const tmp: [1]u8 = [ch];
        _.appendSlice(allocator, tmp[0..1])?;
    }

    /// 追加字节切片；超过内联阈值时自动透明迁移到堆上
    fn appendSlice(*_, allocator: std.mem.Allocator, extra: string)! {
        if (extra.len == 0) {
            return;
        }

        if (buf := _.heap_buf) {
            const new_len = _.heap_len + extra.len;
            if (new_len <= buf.len) {
                @memcpy(buf[_.heap_len..new_len], extra);
                _.heap_len = new_len;
                return;
            }
            const doubled = buf.len * 2;
            const new_cap = if (doubled > new_len) doubled else new_len;
            const alloc_to_use = _.allocator ?? allocator;
            const new_buf = alloc_to_use.alloc(u8, new_cap)?;
            @memcpy(new_buf[0.._.heap_len], buf[0.._.heap_len]);
            @memcpy(new_buf[_.heap_len..new_len], extra);
            alloc_to_use.free(buf);
            _.allocator = alloc_to_use;
            _.heap_buf = new_buf;
            _.heap_len = new_len;
            return;
        }

        const cur_len = _.inline_len as usize;
        const total = cur_len + extra.len;
        if (total <= INLINE_MAX_LEN) {
            @memcpy(_.inline_buf[cur_len..total], extra);
            _.inline_len = total as u8;
            return;
        }

        const init_cap = if (total < 32) @as(usize, 32) else total;
        const new_buf = allocator.alloc(u8, init_cap)?;
        if (cur_len > 0) {
            @memcpy(new_buf[0..cur_len], _.inline_buf[0..cur_len]);
        }
        @memcpy(new_buf[cur_len..total], extra);
        _.allocator = allocator;
        _.heap_buf = new_buf;
        _.heap_len = total;
        _.inline_len = 0;
    }
}
