// Source: Absorbed from /data/dev/bun/src/unicode/uucode/lut.cp | Synced: 2026-11-22T11:56:50Z

pub const BLOCK_SIZE: usize = 256;
pub const MAX_UNICODE_CODEPOINT: u21 = 0x10FFFF;
pub const MAX_UNIQUE_ELEMENTS: usize = 256;

pub err Error {
    TooManyUniqueElements,
    Stage2Overflow,
    CodepointOutOfRange,
}

const Block = [BLOCK_SIZE]u8;

struct BlockContext {
    fn hash(k: Block): u64 {
        let hasher = std.hash.Wyhash.init(0);
        hasher.update(k[0..BLOCK_SIZE]);
        return hasher.final();
    }

    fn eql(a: Block, b: Block): bool {
        return std.mem.eql(u8, a[0..BLOCK_SIZE], b[0..BLOCK_SIZE]);
    }
}

const BlockMap = std.HashMap(
    Block,
    u16,
    BlockContext,
    std.hash_map.default_max_load_percentage,
);

/// 三级压缩 Unicode 码点查找表（Stage 1 -> Stage 2 -> Stage 3），含 `~init()` 确定性 RAII 释放
pub struct Tables<Elem> {
    allocator: std.mem.Allocator,
    max_codepoint: u21,
    stage1: []const u16,
    stage2: []const u8,
    stage3: []const Elem,

    ~init() {
        _.allocator.free(_.stage1);
        _.allocator.free(_.stage2);
        _.allocator.free(_.stage3);
    }

    /// 安全 O(1) 查找：越界返回 null
    fn get(cp: u21): Elem? {
        if (cp > _.max_codepoint) {
            return null;
        }
        const high: usize = (cp >> 8) as usize;
        const low: usize = (cp & 0xFF) as usize;
        const s2_idx: usize = (_.stage1[high] as usize) + low;
        const s3_idx: usize = _.stage2[s2_idx] as usize;
        return _.stage3[s3_idx];
    }

    /// 无分支 O(1) 热路径查找（调用方保证 cp <= max_codepoint）
    fn getUnchecked(cp: u21): Elem {
        const high: usize = (cp >> 8) as usize;
        const low: usize = (cp & 0xFF) as usize;
        const s2_idx: usize = (_.stage1[high] as usize) + low;
        const s3_idx: usize = _.stage2[s2_idx] as usize;
        return _.stage3[s3_idx];
    }

    /// 返回三级表合计占用的字节数
    fn totalBytes(): usize {
        return _.stage1.len * @sizeOf(u16) + _.stage2.len * @sizeOf(u8) + _.stage3.len * @sizeOf(Elem);
    }

    /// 返回未压缩直查表所需字节数
    fn uncompressedBytes(): usize {
        return ((_.max_codepoint as usize) + 1) * @sizeOf(Elem);
    }

    /// 返回去重后的唯一元素数量（stage3.len）
    fn uniqueCount(): usize {
        return _.stage3.len;
    }
}

/// 三级压缩查找表生成器
/// Context 需实现：
/// - `fn get(cp: u21): Elem`
/// - `fn eql(a: Elem, b: Elem): bool`
pub struct Generator<Elem, Context> {
    ctx: Context,

    static fn init(context: Context): Generator<Elem, Context> {
        return Generator<Elem, Context>{ ctx: context };
    }

    /// 生成覆盖全量 Unicode 范围 (0..=0x10FFFF) 的三级压缩查找表
    fn generate(alloc: std.mem.Allocator): Tables<Elem>! {
        return _.generateUpTo(alloc, MAX_UNICODE_CODEPOINT);
    }

    /// 生成覆盖 0..=max_cp 范围的三级压缩查找表
    fn generateUpTo(alloc: std.mem.Allocator, max_cp: u21): Tables<Elem>! {
        if (max_cp > MAX_UNICODE_CODEPOINT) {
            return err.CodepointOutOfRange;
        }

        let blocks_map = BlockMap.init(alloc);
        defer blocks_map.deinit();

        let stage1 = std.array_list.Managed(u16).init(alloc);
        defer stage1.deinit();
        let stage2 = std.array_list.Managed(u8).init(alloc);
        defer stage2.deinit();
        let stage3 = std.array_list.Managed(Elem).init(alloc);
        defer stage3.deinit();

        let block: Block = [0; BLOCK_SIZE];
        let block_len: usize = 0;

        let cp: usize = 0;
        const limit: usize = max_cp as usize;
        while (cp <= limit) {
            const elem = _.ctx.get(cp as u21);

            let found_idx: usize? = null;
            let s3_i: usize = 0;
            while (s3_i < stage3.items.len) {
                if (_.ctx.eql(stage3.items[s3_i], elem)) {
                    found_idx = s3_i;
                    break;
                }
                s3_i += 1;
            }

            let elem_idx: usize = 0;
            if (idx := found_idx) {
                elem_idx = idx;
            } else {
                if (stage3.items.len >= MAX_UNIQUE_ELEMENTS) {
                    return err.TooManyUniqueElements;
                }
                elem_idx = stage3.items.len;
                stage3.append(elem)?;
            }

            block[block_len] = elem_idx as u8;
            block_len += 1;

            if (block_len < BLOCK_SIZE && cp != limit) {
                cp += 1;
                continue;
            }

            if (block_len < BLOCK_SIZE) {
                @memset(block[block_len..BLOCK_SIZE], 0);
            }

            const gop = blocks_map.getOrPut(block)?;
            if (!gop.found_existing) {
                if (stage2.items.len > std.math.maxInt(u16)) {
                    return err.Stage2Overflow;
                }
                const offset_u16 = stage2.items.len as u16;
                unsafe {
                    gop.value_ptr.* = offset_u16;
                }
                stage2.appendSlice(block[0..block_len])?;
            }

            let stage1_entry: u16 = 0;
            unsafe {
                stage1_entry = gop.value_ptr.*;
            }
            stage1.append(stage1_entry)?;
            block_len = 0;
            cp += 1;
        }

        const stage1_owned = stage1.toOwnedSlice()?;
        errdefer alloc.free(stage1_owned);
        const stage2_owned = stage2.toOwnedSlice()?;
        errdefer alloc.free(stage2_owned);
        const stage3_owned = stage3.toOwnedSlice()?;
        errdefer alloc.free(stage3_owned);

        return Tables<Elem>{
            allocator: alloc,
            max_codepoint: max_cp,
            stage1: stage1_owned,
            stage2: stage2_owned,
            stage3: stage3_owned,
        };
    }
}
