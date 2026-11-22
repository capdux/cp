// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/renameSymbolsInChunk.cp | Synced: 2026-11-21T23:23:20Z
// Ported from Bun: /data/dev/bun/src/bundler/linker_context/renameSymbolsInChunk.cp & /data/dev/bun/docs/bundler/minifier.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation Chunk-Level Symbol Renaming Coordinator:
// - Deterministically sorts cross-chunk imports by (stable_source_index, inner_index)
// - Merges nested scope slot high-water marks via `ChunkSlotCounts.unionMax`
// - Aggregates source character frequencies (`ChunkCharFreq`) to produce compression-friendly identifier alphabets
// - Coordinates both frequency-driven minified renaming (`minify_identifiers=true`) and collision-free numbered renaming (`minify_identifiers=false`)
// - Respects JS reserved words, CJS reserved globals (`exports`/`module`/`require`), and CJS vs ESM module wrapper scoping.

import {
    isJsReservedWord,
    isCjsReservedName,
    encodeMinifiedNameWithAlphabet,
} from "./renamer.cp";

/// 单个 Chunk 最多协调的符号数量
pub const CHUNK_RENAME_MAX_SYMBOLS: usize = 64;

/// 单个 Chunk 最多收集的自定义保留标识符数量
pub const CHUNK_RENAME_MAX_RESERVED: usize = 32;

/// 重命名后单符号最大字节长度
pub const CHUNK_RENAME_NAME_CAP: usize = 48;

/// Chunk 级符号重命名错误集
pub err ChunkRenameError {
    TooManySymbols,
    TooManyReserved,
    EmptySymbolName,
    NameTooLong,
    DuplicateSymbolRef,
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

/// 跨模块稳定符号引用键（对齐 Bun `StableRef`）
pub struct ChunkStableRef {
    stable_source_index: u32 = 0,
    inner_index: u32 = 0,

    fn eql(other: ChunkStableRef): bool {
        return _.stable_source_index == other.stable_source_index &&
            _.inner_index == other.inner_index;
    }

    /// 确定性字典序比较，消除并行解析导致的命名漂移
    fn lessThan(other: ChunkStableRef): bool {
        if (_.stable_source_index != other.stable_source_index) {
            return _.stable_source_index < other.stable_source_index;
        }
        return _.inner_index < other.inner_index;
    }
}

/// 嵌套作用域槽位计数上界（对齐 Bun `js_ast.SlotCounts`）
pub struct ChunkSlotCounts {
    default_slots: u32 = 0,
    label_slots: u32 = 0,
    must_not_be_renamed_slots: u32 = 0,

    fn unionMax(*_, other: ChunkSlotCounts) {
        if (other.default_slots > _.default_slots) { _.default_slots = other.default_slots; }
        if (other.label_slots > _.label_slots) { _.label_slots = other.label_slots; }
        if (other.must_not_be_renamed_slots > _.must_not_be_renamed_slots) {
            _.must_not_be_renamed_slots = other.must_not_be_renamed_slots;
        }
    }
}

/// 源码标识符字符频次统计表（按频次排序生成对 Gzip/Brotli 压缩最友好的短名字母表）
pub struct ChunkCharFreq {
    /// 64 个合法 JS 标识符字符的频次槽：0..25='a'..'z', 26..51='A'..'Z', 52='_', 53='$', 54..63='0'..'9'
    freqs: [64]u32 = [0; 64],

    static fn init(): ChunkCharFreq {
        return ChunkCharFreq{};
    }

    static fn charToSlot(c: u8): usize? {
        if (c >= 'a' && c <= 'z') { return @as(usize, c - 'a'); }
        if (c >= 'A' && c <= 'Z') { return @as(usize, 26 + (c - 'A')); }
        if (c == '_') { return 52; }
        if (c == '$') { return 53; }
        if (c >= '0' && c <= '9') { return @as(usize, 54 + (c - '0')); }
        return null;
    }

    static fn slotToChar(slot: usize): u8 {
        if (slot < 26) { return @as(u8, @intCast(slot)) + 'a'; }
        if (slot < 52) { return @as(u8, @intCast(slot - 26)) + 'A'; }
        if (slot == 52) { return '_'; }
        if (slot == 53) { return '$'; }
        return @as(u8, @intCast(slot - 54)) + '0';
    }

    /// 扫描源码片段累加字符出现频次
    fn scan(*_, text: []const u8) {
        let i: usize = 0;
        while (i < text.len) {
            if (slot := ChunkCharFreq.charToSlot(text[i])) {
                _.freqs[slot] += 1;
            }
            i += 1;
        }
    }

    /// 合并另一文件的字符频次统计（对应 Bun `freq.include(char_freqs[i])`）
    fn include(*_, other: ChunkCharFreq) {
        let i: usize = 0;
        while (i < 64) {
            _.freqs[i] += other.freqs[i];
            i += 1;
        }
    }

    /// 生成按频次降序排列的 54 字节首字符表与 64 字节尾字符表
    fn compileAlphabets(head_out: *[54]u8, tail_out: *[64]u8) {
        // 1. 首字符表（0..54 不含数字）
        let head_slots: [54]usize = [0; 54];
        let hi: usize = 0;
        while (hi < 54) {
            head_slots[hi] = hi;
            hi += 1;
        }
        let hs: usize = 1;
        while (hs < 54) {
            const cur = head_slots[hs];
            let hj = hs;
            while (hj > 0 && _.freqs[cur] > _.freqs[head_slots[hj - 1]]) {
                head_slots[hj] = head_slots[hj - 1];
                hj -= 1;
            }
            head_slots[hj] = cur;
            hs += 1;
        }
        let ho: usize = 0;
        while (ho < 54) {
            head_out[ho] = ChunkCharFreq.slotToChar(head_slots[ho]);
            ho += 1;
        }

        // 2. 尾字符表（0..64 含数字）
        let tail_slots: [64]usize = [0; 64];
        let ti: usize = 0;
        while (ti < 64) {
            tail_slots[ti] = ti;
            ti += 1;
        }
        let ts: usize = 1;
        while (ts < 64) {
            const cur = tail_slots[ts];
            let tj = ts;
            while (tj > 0 && _.freqs[cur] > _.freqs[tail_slots[tj - 1]]) {
                tail_slots[tj] = tail_slots[tj - 1];
                tj -= 1;
            }
            tail_slots[tj] = cur;
            ts += 1;
        }
        let to: usize = 0;
        while (to < 64) {
            tail_out[to] = ChunkCharFreq.slotToChar(tail_slots[to]);
            to += 1;
        }
    }
}

/// Chunk 符号来源类别（对应 Bun `renameSymbolsInChunk` 对跨 Chunk 导入、CJS 包装与顶层符号的区分）
pub enum ChunkRenameSymbolKind: u8 {
    TopLevel = 0,
    CrossChunkImport = 1,
    CjsWrapper = 2,
    CjsHoistedExternalImport = 3,
}

/// Chunk 符号输入条目
pub struct ChunkRenameSymbolInput {
    sym_ref: ChunkStableRef = ChunkStableRef{},
    original_name: []const u8 = "",
    use_count: u32 = 1,
    kind: ChunkRenameSymbolKind = .TopLevel,
    must_not_be_renamed: bool = false,
}

/// 重命名后的固定缓冲区名称
pub struct ChunkRenamedName {
    buf: [CHUNK_RENAME_NAME_CAP]u8 = [0; CHUNK_RENAME_NAME_CAP],
    len: usize = 0,

    fn asSlice(): []const u8 {
        return _.buf[0.._.len];
    }

    fn eql(other: []const u8): bool {
        return eqlBytes(_.buf[0.._.len], other);
    }
}

/// 单符号重命名结果记录
pub struct ChunkRenameEntry {
    sym_ref: ChunkStableRef = ChunkStableRef{},
    original_name: []const u8 = "",
    use_count: u32 = 0,
    assigned_slot: u32 = 0,
    renamed: ChunkRenamedName = ChunkRenamedName{},
}

/// Chunk 重命名配置选项
pub struct ChunkRenameOptions {
    minify_identifiers: bool = false,
    is_cjs_output: bool = false,
    /// 是否使用字符频次驱动的自定义字母表（仅 `minify_identifiers=true` 时生效）
    use_char_freq_alphabet: bool = true,
}

/// Chunk 符号重命名完整结果
pub struct ChunkRenameResult {
    entry_count: usize = 0,
    entries: [CHUNK_RENAME_MAX_SYMBOLS]ChunkRenameEntry = [ChunkRenameEntry{}; CHUNK_RENAME_MAX_SYMBOLS],
    starting_top_level_slot: u32 = 0,

    fn slice(): []const ChunkRenameEntry {
        return _.entries[0.._.entry_count];
    }

    /// 按 `ChunkStableRef` 查找最终分配的标识符名称
    fn lookupName(sym_ref: ChunkStableRef): ([]const u8)? {
        let i: usize = 0;
        while (i < _.entry_count) {
            if (_.entries[i].sym_ref.eql(sym_ref)) {
                return _.entries[i].renamed.asSlice();
            }
            i += 1;
        }
        return null;
    }
}

fn copyIntoRenamed(src: []const u8): ChunkRenameError!ChunkRenamedName {
    if (src.len > CHUNK_RENAME_NAME_CAP) {
        return ChunkRenameError.NameTooLong;
    }
    let out = ChunkRenamedName{ len: src.len };
    let i: usize = 0;
    while (i < src.len) {
        out.buf[i] = src[i];
        i += 1;
    }
    return out;
}

fn formatNumberedCandidate(base: []const u8, suffix_num: u32): ChunkRenameError!ChunkRenamedName {
    let dec_tmp: [12]u8 = [0; 12];
    let dlen: usize = 0;
    let v = suffix_num;
    if (v == 0) {
        dec_tmp[0] = '0';
        dlen = 1;
    } else {
        while (v > 0 && dlen < 12) {
            dec_tmp[dlen] = @as(u8, @intCast(v % 10)) + '0';
            v = v / 10;
            dlen += 1;
        }
    }
    if (base.len + dlen > CHUNK_RENAME_NAME_CAP) {
        return ChunkRenameError.NameTooLong;
    }
    let out = ChunkRenamedName{ len: base.len + dlen };
    let i: usize = 0;
    while (i < base.len) {
        out.buf[i] = base[i];
        i += 1;
    }
    let di: usize = dlen;
    let pos = base.len;
    while (di > 0) {
        di -= 1;
        out.buf[pos] = dec_tmp[di];
        pos += 1;
    }
    return out;
}

/// 零堆分配 Chunk 级符号重命名协调器（对齐 Bun `renameSymbolsInChunk.cp`）
pub struct ChunkSymbolRenamer {
    symbol_count: usize = 0,
    symbols: [CHUNK_RENAME_MAX_SYMBOLS]ChunkRenameSymbolInput = [ChunkRenameSymbolInput{}; CHUNK_RENAME_MAX_SYMBOLS],
    reserved_count: usize = 0,
    reserved_names: [CHUNK_RENAME_MAX_RESERVED]([]const u8) = [""; CHUNK_RENAME_MAX_RESERVED],
    nested_slots: ChunkSlotCounts = ChunkSlotCounts{},
    char_freq: ChunkCharFreq = ChunkCharFreq.init(),

    static fn init(): ChunkSymbolRenamer {
        return ChunkSymbolRenamer{};
    }

    fn addReservedName(*_, name: []const u8): ChunkRenameError!bool {
        if (name.len == 0) { return ChunkRenameError.EmptySymbolName; }
        let i: usize = 0;
        while (i < _.reserved_count) {
            if (eqlBytes(_.reserved_names[i], name)) { return false; }
            i += 1;
        }
        if (_.reserved_count >= CHUNK_RENAME_MAX_RESERVED) {
            return ChunkRenameError.TooManyReserved;
        }
        _.reserved_names[_.reserved_count] = name;
        _.reserved_count += 1;
        return true;
    }

    fn mergeNestedSlots(*_, slots: ChunkSlotCounts) {
        _.nested_slots.unionMax(slots);
    }

    fn includeCharFreq(*_, freq: ChunkCharFreq) {
        _.char_freq.include(freq);
    }

    /// 注册待重命名的 Chunk 符号（重复 `sym_ref` 拒绝）
    fn addSymbol(*_, input: ChunkRenameSymbolInput): ChunkRenameError!usize {
        if (input.original_name.len == 0) {
            return ChunkRenameError.EmptySymbolName;
        }
        let i: usize = 0;
        while (i < _.symbol_count) {
            if (_.symbols[i].sym_ref.eql(input.sym_ref)) {
                return ChunkRenameError.DuplicateSymbolRef;
            }
            i += 1;
        }
        if (_.symbol_count >= CHUNK_RENAME_MAX_SYMBOLS) {
            return ChunkRenameError.TooManySymbols;
        }
        const slot = _.symbol_count;
        _.symbols[slot] = input;
        _.symbol_count += 1;
        return slot;
    }

    fn isNameForbidden(name: []const u8, is_cjs: bool, used: []const ChunkRenameEntry): bool {
        if (isJsReservedWord(name)) { return true; }
        if (is_cjs && isCjsReservedName(name)) { return true; }
        let ri: usize = 0;
        while (ri < _.reserved_count) {
            if (eqlBytes(_.reserved_names[ri], name)) { return true; }
            ri += 1;
        }
        let ui: usize = 0;
        while (ui < used.len) {
            if (used[ui].renamed.eql(name)) { return true; }
            ui += 1;
        }
        return false;
    }

    /// 执行 Chunk 级符号重命名（支持 Minify 频次短名模式与非压缩数字后缀冲突消解模式）
    fn rename(opts: ChunkRenameOptions): ChunkRenameError!ChunkRenameResult {
        let out = ChunkRenameResult{
            starting_top_level_slot: _.nested_slots.default_slots,
        };
        if (_.symbol_count == 0) { return out; }

        // 构造确定性处理顺序索引数组
        let order: [CHUNK_RENAME_MAX_SYMBOLS]usize = [0; CHUNK_RENAME_MAX_SYMBOLS];
        let oi: usize = 0;
        while (oi < _.symbol_count) {
            order[oi] = oi;
            oi += 1;
        }

        if (opts.minify_identifiers) {
            // Minify 模式：按 use_count 降序，同频按 StableRef 升序
            let i: usize = 1;
            while (i < _.symbol_count) {
                const cur = order[i];
                let j = i;
                while (j > 0) {
                    const prev = order[j - 1];
                    let before = false;
                    if (_.symbols[cur].use_count != _.symbols[prev].use_count) {
                        before = _.symbols[cur].use_count > _.symbols[prev].use_count;
                    } else {
                        before = _.symbols[cur].sym_ref.lessThan(_.symbols[prev].sym_ref);
                    }
                    if (!before) { break; }
                    order[j] = prev;
                    j -= 1;
                }
                order[j] = cur;
                i += 1;
            }
        } else {
            // 非压缩模式：跨 Chunk 导入符号优先按 StableRef 升序，其余按 StableRef 升序
            let i: usize = 1;
            while (i < _.symbol_count) {
                const cur = order[i];
                let j = i;
                while (j > 0) {
                    const prev = order[j - 1];
                    const cur_imp = _.symbols[cur].kind == .CrossChunkImport;
                    const prev_imp = _.symbols[prev].kind == .CrossChunkImport;
                    let before = false;
                    if (cur_imp && !prev_imp) {
                        before = true;
                    } else if (!cur_imp && prev_imp) {
                        before = false;
                    } else {
                        before = _.symbols[cur].sym_ref.lessThan(_.symbols[prev].sym_ref);
                    }
                    if (!before) { break; }
                    order[j] = prev;
                    j -= 1;
                }
                order[j] = cur;
                i += 1;
            }
        }

        let head_alpha: [54]u8 = [0; 54];
        let tail_alpha: [64]u8 = [0; 64];
        if (opts.minify_identifiers) {
            if (opts.use_char_freq_alphabet) {
                _.char_freq.compileAlphabets(&head_alpha, &tail_alpha);
            } else {
                const default_freq = ChunkCharFreq.init();
                default_freq.compileAlphabets(&head_alpha, &tail_alpha);
            }
        }

        // 先锁定 must_not_be_renamed 符号，防止短名分配与之冲突
        let k0: usize = 0;
        while (k0 < _.symbol_count) {
            const sym = _.symbols[order[k0]];
            if (sym.must_not_be_renamed) {
                const fixed_name = copyIntoRenamed(sym.original_name)?;
                out.entries[out.entry_count] = ChunkRenameEntry{
                    sym_ref: sym.sym_ref,
                    original_name: sym.original_name,
                    use_count: sym.use_count,
                    assigned_slot: 0,
                    renamed: fixed_name,
                };
                out.entry_count += 1;
            }
            k0 += 1;
        }

        let next_slot: u64 = @as(u64, _.nested_slots.default_slots);
        let k: usize = 0;
        while (k < _.symbol_count) {
            const sym = _.symbols[order[k]];
            if (sym.must_not_be_renamed) {
                k += 1;
                continue;
            }

            if (opts.minify_identifiers) {
                let cand_buf: [CHUNK_RENAME_NAME_CAP]u8 = [0; CHUNK_RENAME_NAME_CAP];
                let assigned_name = ChunkRenamedName{};
                let assigned_slot_u32: u32 = 0;

                while (true) {
                    const encoded = encodeMinifiedNameWithAlphabet(
                        next_slot,
                        head_alpha[0..],
                        tail_alpha[0..],
                        cand_buf[0..],
                    ) catch return ChunkRenameError.NameTooLong;
                    const cur_slot = next_slot;
                    next_slot += 1;
                    if (!_.isNameForbidden(encoded, opts.is_cjs_output, out.entries[0..out.entry_count])) {
                        assigned_name = copyIntoRenamed(encoded)?;
                        assigned_slot_u32 = @as(u32, @intCast(cur_slot));
                        break;
                    }
                }

                out.entries[out.entry_count] = ChunkRenameEntry{
                    sym_ref: sym.sym_ref,
                    original_name: sym.original_name,
                    use_count: sym.use_count,
                    assigned_slot: assigned_slot_u32,
                    renamed: assigned_name,
                };
                out.entry_count += 1;
            } else {
                // 非压缩模式：保留原名，若冲突或命中保留词则递增附加 2, 3, ...
                let assigned_name = ChunkRenamedName{};
                if (!_.isNameForbidden(sym.original_name, opts.is_cjs_output, out.entries[0..out.entry_count])) {
                    assigned_name = copyIntoRenamed(sym.original_name)?;
                } else {
                    let suffix: u32 = 2;
                    while (true) {
                        const cand = formatNumberedCandidate(sym.original_name, suffix)?;
                        if (!_.isNameForbidden(cand.asSlice(), opts.is_cjs_output, out.entries[0..out.entry_count])) {
                            assigned_name = cand;
                            break;
                        }
                        suffix += 1;
                    }
                }

                out.entries[out.entry_count] = ChunkRenameEntry{
                    sym_ref: sym.sym_ref,
                    original_name: sym.original_name,
                    use_count: sym.use_count,
                    assigned_slot: 0,
                    renamed: assigned_name,
                };
                out.entry_count += 1;
            }
            k += 1;
        }

        return out;
    }
}
