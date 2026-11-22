// Source: Absorbed from /data/dev/bun/src/js_parser/ast/ServerComponentBoundary.cp | Synced: 2026-11-20T13:19:13Z
// Pure-CP zero-allocation React Server Components (RSC) Boundary Registry:
// - Parses `"use client"` / `"use server"` module directives
// - Tracks canonical `source_index` <-> cross-platform `reference_source_index`
//   and optional `ssr_source_index` (separate SSR graph mode)
// - Provides O(1) bitset construction (`ScbBitSet`, up to 1024 modules, zero alloc)
//   and duplicate/capacity guards.

/// 最大边界条目容量与零堆分配位图上限（16 * 64 = 1024 个模块索引）
pub const MAX_SCB_BOUNDARIES: usize = 128;
pub const MAX_SCB_BITSET_WORDS: usize = 16;
pub const MAX_SCB_BITSET_BITS: u32 = 1024;

/// Server Component Boundary 注册表错误集
pub err ScbError {
    DuplicateSourceIndex,
    BoundaryCapacityExceeded,
    SourceIndexOutOfRange,
}

/// 模块顶层指令类型（对齐 Bun `js_ast.UseDirective`）
pub enum UseDirective: u8 {
    none = 0,
    use_client = 1,
    use_server = 2,

    /// 解析指令字面量（支持裸串 `"use client"` 或带单/双引号及空白的源码片段）
    static fn parse(raw: string): UseDirective {
        let start: usize = 0;
        let end: usize = raw.len;
        while (start < end && (raw[start] == ' ' || raw[start] == '\t' || raw[start] == '\r' || raw[start] == '\n')) {
            start += 1;
        }
        while (end > start && (raw[end - 1] == ' ' || raw[end - 1] == '\t' || raw[end - 1] == '\r' || raw[end - 1] == '\n' || raw[end - 1] == ';')) {
            end -= 1;
        }
        if (end - start >= 2) {
            const q0 = raw[start];
            const q1 = raw[end - 1];
            if ((q0 == '"' && q1 == '"') || (q0 == '\'' && q1 == '\'')) {
                start += 1;
                end -= 1;
            }
        }
        const inner = raw[start..end];
        if (std.mem.eql(u8, inner, "use client")) {
            return .use_client;
        }
        if (std.mem.eql(u8, inner, "use server")) {
            return .use_server;
        }
        return .none;
    }

    fn asStr(): string {
        if (_ == .use_client) {
            return "use client";
        }
        if (_ == .use_server) {
            return "use server";
        }
        return "";
    }

    fn isBoundary(): bool {
        return _ == .use_client || _ == .use_server;
    }
}

/// 单个客户端/服务端边界描述符（对齐 Bun `ServerComponentBoundary`）
pub struct ServerComponentBoundary {
    use_directive: UseDirective = .none,
    /// 原始模块在模块图中的索引
    source_index: u32 = 0,
    /// 对端平台生成的引用代理模块索引（client component -> server 侧引用；server action -> client 侧引用）
    reference_source_index: u32 = 0,
    /// 独立 SSR 图开启时指向独立 SSR 模块，否则等于 `reference_source_index`
    ssr_source_index: u32 = 0,

    fn hasSeparateSsr(): bool {
        return _.ssr_source_index != _.reference_source_index;
    }
}

/// 零堆分配边界索引位图（对齐 Bun `ServerComponentBoundarySlice.bitSet`，支持 0..1023）
pub struct ScbBitSet {
    words: [MAX_SCB_BITSET_WORDS]u64 = [0; MAX_SCB_BITSET_WORDS],

    static fn init(): ScbBitSet {
        return ScbBitSet{
            words: [0; MAX_SCB_BITSET_WORDS],
        };
    }

    fn set(*_, source_index: u32): ! {
        if (source_index >= MAX_SCB_BITSET_BITS) {
            return err.SourceIndexOutOfRange;
        }
        const word_idx: usize = @intCast(source_index / 64);
        const bit_idx: u6 = @intCast(source_index % 64);
        _.words[word_idx] |= (@as(u64, 1) << bit_idx);
    }

    fn isSet(source_index: u32): bool {
        if (source_index >= MAX_SCB_BITSET_BITS) {
            return false;
        }
        const word_idx: usize = @intCast(source_index / 64);
        const bit_idx: u6 = @intCast(source_index % 64);
        return ((_.words[word_idx] >> bit_idx) & 1) != 0;
    }

    fn count(): u32 {
        let total: u32 = 0;
        let w: usize = 0;
        while (w < MAX_SCB_BITSET_WORDS) {
            let v = _.words[w];
            while (v != 0) {
                v = v & (v - 1);
                total += 1;
            }
            w += 1;
        }
        return total;
    }
}

/// Server Component Boundary 注册表与快速查询器（对齐 Bun `ServerComponentBoundary.List`）
pub struct ServerComponentBoundaryTable {
    items: [MAX_SCB_BOUNDARIES]ServerComponentBoundary = [ServerComponentBoundary{}; MAX_SCB_BOUNDARIES],
    len: usize = 0,

    static fn init(): ServerComponentBoundaryTable {
        return ServerComponentBoundaryTable{
            items: [ServerComponentBoundary{}; MAX_SCB_BOUNDARIES],
            len: 0,
        };
    }

    fn isEmpty(): bool {
        return _.len == 0;
    }

    /// 按原始 `source_index` 查找条目下标（对齐 Bun `getIndex`）
    fn getIndex(real_source_index: u32): usize? {
        let i: usize = 0;
        while (i < _.len) {
            if (_.items[i].source_index == real_source_index) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// 注册新边界（禁止同一 `source_index` 重复注册）
    fn put(
        *_,
        source_index: u32,
        use_directive: UseDirective,
        reference_source_index: u32,
        ssr_source_index: u32,
    ): usize! {
        if (_.getIndex(source_index) != null) {
            return err.DuplicateSourceIndex;
        }
        if (_.len >= MAX_SCB_BOUNDARIES) {
            return err.BoundaryCapacityExceeded;
        }
        const idx = _.len;
        _.items[idx] = ServerComponentBoundary{
            use_directive: use_directive,
            source_index: source_index,
            reference_source_index: reference_source_index,
            ssr_source_index: ssr_source_index,
        };
        _.len += 1;
        return idx;
    }

    /// 注册共享 SSR 引用的边界（`ssr_source_index == reference_source_index`）
    fn putSharedSsr(
        *_,
        source_index: u32,
        use_directive: UseDirective,
        reference_source_index: u32,
    ): usize! {
        return _.put(source_index, use_directive, reference_source_index, reference_source_index);
    }

    fn get(real_source_index: u32): ServerComponentBoundary? {
        if (idx := _.getIndex(real_source_index)) {
            return _.items[idx];
        }
        return null;
    }

    /// 查询对端引用代理模块索引（对齐 Bun `getReferenceSourceIndex`）
    fn getReferenceSourceIndex(real_source_index: u32): u32? {
        if (idx := _.getIndex(real_source_index)) {
            return _.items[idx].reference_source_index;
        }
        return null;
    }

    /// 查询 SSR 图模块索引
    fn getSsrSourceIndex(real_source_index: u32): u32? {
        if (idx := _.getIndex(real_source_index)) {
            return _.items[idx].ssr_source_index;
        }
        return null;
    }

    /// 按对端引用模块索引反向查找原始边界
    fn getByReferenceIndex(ref_source_index: u32): ServerComponentBoundary? {
        let i: usize = 0;
        while (i < _.len) {
            if (_.items[i].reference_source_index == ref_source_index) {
                return _.items[i];
            }
            i += 1;
        }
        return null;
    }

    fn countByDirective(directive: UseDirective): u32 {
        let total: u32 = 0;
        let i: usize = 0;
        while (i < _.len) {
            if (_.items[i].use_directive == directive) {
                total += 1;
            }
            i += 1;
        }
        return total;
    }

    /// 生成所有已注册边界 `source_index` 的紧凑位图（对齐 Bun `bitSet`）
    fn buildBitSet(): ScbBitSet! {
        let bs = ScbBitSet.init();
        let i: usize = 0;
        while (i < _.len) {
            bs.set(_.items[i].source_index)?;
            i += 1;
        }
        return bs;
    }
}
