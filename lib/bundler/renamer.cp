// Source: Absorbed from /data/dev/bun/src/js_printer/renamer.cp | Synced: 2026-11-22T06:05:31Z
// Pure-CP zero-allocation JS Bundler Symbol Renamer & Minifier:
// - ECMAScript reserved word & CJS/ESM reserved identifier checks
// - Base54/Base64 minified identifier encoder (supports custom char-freq alphabets)
// - 15-byte inline name storage (`InlineName`, mirroring Bun `InlineString`)
// - Scope-aware collision-avoidance `NumberScopeRenamer` (`foo`, `foo2`, `foo3`...)
// - Frequency-ordered `MinifySlotAssigner` with JSX capital-letter & namespace rules

/// 单个内联短名称最大字节数（对齐 Bun `InlineString` 的 15 字节槽位）
pub const INLINE_NAME_CAP: usize = 15;

/// `NumberScopeRenamer` 单作用域最大记录条目数（静态零堆分配）
pub const MAX_SCOPE_NAMES: usize = 128;

/// `MinifySlotAssigner` 最大保留名容量与最大待分配符号槽位数
pub const MAX_RESERVED_NAMES: usize = 64;
pub const MAX_MINIFY_SLOTS: usize = 128;

/// 默认 JS 标识符首字符表（54 字符：a-z, A-Z, _, $）
pub const DEFAULT_HEAD_ALPHABET: string = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_$";

/// 默认 JS 标识符后续字符表（64 字符：a-z, A-Z, _, $, 0-9）
pub const DEFAULT_TAIL_ALPHABET: string = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_$0123456789";

/// 重命名器错误集
pub err RenamerError {
    BufferTooSmall,
    ScopeCapacityExceeded,
    ReservedCapacityExceeded,
    SlotCapacityExceeded,
    NameTooLong,
}

/// 符号重命名命名空间（对齐 Bun `js_ast.Symbol.SlotNamespace`）
pub enum SlotNamespace: u8 {
    /// 普通变量/函数/类标识符：不可与 JS 保留字或作用域保留名冲突
    default = 0,
    /// 语句标签：仅不可与 JS 关键字冲突
    label = 1,
    /// 类私有成员 `#x`：因带 `#` 前缀，不与关键字或普通标识符冲突
    private_name = 2,
}

/// 15 字节栈上内联短字符串（对齐 Bun `InlineString`，零堆分配）
pub struct InlineName {
    bytes: [INLINE_NAME_CAP]u8 = [0; INLINE_NAME_CAP],
    len: u8 = 0,

    static fn empty(): InlineName {
        return InlineName{
            bytes: [0; INLINE_NAME_CAP],
            len: 0,
        };
    }

    static fn fromSlice(s: string): InlineName! {
        if (s.len > INLINE_NAME_CAP) {
            return err.NameTooLong;
        }
        let out = InlineName.empty();
        out.len = @intCast(s.len);
        let i: usize = 0;
        while (i < s.len) {
            out.bytes[i] = s[i];
            i += 1;
        }
        return out;
    }

    fn slice(): string {
        return _.bytes[0.._.len];
    }

    fn eql(other: string): bool {
        return std.mem.eql(u8, _.slice(), other);
    }
}

/// 判断字符是否为合法 ASCII JS 标识符首字符（`a-z`, `A-Z`, `_`, `$`）
pub fn isJsIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$';
}

/// 判断字符是否为合法 ASCII JS 标识符后续字符（含 `0-9`）
pub fn isJsIdentContinue(c: u8): bool {
    return isJsIdentStart(c) || (c >= '0' && c <= '9');
}

/// 将任意字符串清洗为合法 ASCII JS 标识符（非法字符替换为 `_`，数字开头加 `_` 前缀，空串转为 `_`）
pub fn ensureValidJsIdentifier(raw: string, out: []u8): string! {
    if (raw.len == 0) {
        if (out.len < 1) {
            return err.BufferTooSmall;
        }
        out[0] = '_';
        return out[0..1];
    }
    const need_prefix = !isJsIdentStart(raw[0]) && (raw[0] >= '0' && raw[0] <= '9');
    const total = raw.len + (if (need_prefix) @as(usize, 1) else @as(usize, 0));
    if (out.len < total) {
        return err.BufferTooSmall;
    }
    let pos: usize = 0;
    if (need_prefix) {
        out[pos] = '_';
        pos += 1;
    }
    let i: usize = 0;
    while (i < raw.len) {
        const c = raw[i];
        if (pos == 0) {
            out[pos] = if (isJsIdentStart(c)) c else '_';
        } else {
            out[pos] = if (isJsIdentContinue(c)) c else '_';
        }
        pos += 1;
        i += 1;
    }
    return out[0..pos];
}

/// 判断是否为 ECMAScript 关键字 / 严格模式保留字
pub fn isJsReservedWord(name: string): bool {
    if (name.len < 2 || name.len > 10) {
        return false;
    }
    if (name.len == 2) {
        return std.mem.eql(u8, name, "do") ||
            std.mem.eql(u8, name, "if") ||
            std.mem.eql(u8, name, "in");
    }
    if (name.len == 3) {
        return std.mem.eql(u8, name, "for") ||
            std.mem.eql(u8, name, "let") ||
            std.mem.eql(u8, name, "new") ||
            std.mem.eql(u8, name, "try") ||
            std.mem.eql(u8, name, "var");
    }
    if (name.len == 4) {
        return std.mem.eql(u8, name, "case") ||
            std.mem.eql(u8, name, "else") ||
            std.mem.eql(u8, name, "enum") ||
            std.mem.eql(u8, name, "null") ||
            std.mem.eql(u8, name, "this") ||
            std.mem.eql(u8, name, "true") ||
            std.mem.eql(u8, name, "void") ||
            std.mem.eql(u8, name, "with");
    }
    if (name.len == 5) {
        return std.mem.eql(u8, name, "await") ||
            std.mem.eql(u8, name, "break") ||
            std.mem.eql(u8, name, "catch") ||
            std.mem.eql(u8, name, "class") ||
            std.mem.eql(u8, name, "const") ||
            std.mem.eql(u8, name, "false") ||
            std.mem.eql(u8, name, "super") ||
            std.mem.eql(u8, name, "throw") ||
            std.mem.eql(u8, name, "while") ||
            std.mem.eql(u8, name, "yield");
    }
    if (name.len == 6) {
        return std.mem.eql(u8, name, "delete") ||
            std.mem.eql(u8, name, "export") ||
            std.mem.eql(u8, name, "import") ||
            std.mem.eql(u8, name, "public") ||
            std.mem.eql(u8, name, "return") ||
            std.mem.eql(u8, name, "static") ||
            std.mem.eql(u8, name, "switch") ||
            std.mem.eql(u8, name, "typeof");
    }
    if (name.len == 7) {
        return std.mem.eql(u8, name, "default") ||
            std.mem.eql(u8, name, "extends") ||
            std.mem.eql(u8, name, "finally") ||
            std.mem.eql(u8, name, "package") ||
            std.mem.eql(u8, name, "private");
    }
    if (name.len == 8) {
        return std.mem.eql(u8, name, "continue") ||
            std.mem.eql(u8, name, "debugger") ||
            std.mem.eql(u8, name, "function");
    }
    if (name.len == 9) {
        return std.mem.eql(u8, name, "interface") ||
            std.mem.eql(u8, name, "protected");
    }
    return std.mem.eql(u8, name, "implements") ||
        std.mem.eql(u8, name, "instanceof");
}

/// 判断是否为 CommonJS / 运行时作用域内置保留标识符
pub fn isCjsReservedName(name: string): bool {
    return std.mem.eql(u8, name, "exports") ||
        std.mem.eql(u8, name, "require") ||
        std.mem.eql(u8, name, "module") ||
        std.mem.eql(u8, name, "__filename") ||
        std.mem.eql(u8, name, "__dirname") ||
        std.mem.eql(u8, name, "arguments") ||
        std.mem.eql(u8, name, "eval");
}

/// 将序号 `index` 按 Base54(首字符) + Base64(后续字符) 编码为最短合法 JS 标识符
pub fn encodeMinifiedNameWithAlphabet(
    index: u64,
    head_alphabet: string,
    tail_alphabet: string,
    out: []u8,
): string! {
    if (out.len == 0 || head_alphabet.len == 0 || tail_alphabet.len == 0) {
        return err.BufferTooSmall;
    }
    const h_len: u64 = @intCast(head_alphabet.len);
    const t_len: u64 = @intCast(tail_alphabet.len);

    let pos: usize = 0;
    let n = index;
    const h_idx: usize = @intCast(n % h_len);
    out[pos] = head_alphabet[h_idx];
    pos += 1;
    n = n / h_len;

    while (n > 0) {
        n -= 1;
        if (pos >= out.len) {
            return err.BufferTooSmall;
        }
        const t_idx: usize = @intCast(n % t_len);
        out[pos] = tail_alphabet[t_idx];
        pos += 1;
        n = n / t_len;
    }
    return out[0..pos];
}

/// 使用默认字符表编码最短合法 JS 标识符（0->"a", 53->"$", 54->"aa", ...）
pub fn encodeMinifiedName(index: u64, out: []u8): string! {
    return encodeMinifiedNameWithAlphabet(index, DEFAULT_HEAD_ALPHABET, DEFAULT_TAIL_ALPHABET, out);
}

fn appendU32Decimal(val: u32, out: []u8): usize! {
    let tmp: [12]u8 = [0; 12];
    let len: usize = 0;
    let n = val;
    if (n == 0) {
        tmp[0] = '0';
        len = 1;
    } else {
        while (n > 0) {
            tmp[len] = @intCast('0' + (n % 10));
            len += 1;
            n = n / 10;
        }
    }
    if (out.len < len) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < len) {
        out[i] = tmp[len - 1 - i];
        i += 1;
    }
    return len;
}

/// 单个名称计数条目
struct ScopeNameEntry {
    name: InlineName = InlineName.empty(),
    count: u32 = 0,
}

/// 作用域冲突规避编号重命名器（对齐 Bun `NumberScope.findUnusedName`）
/// - 首次注册 `"foo"` 返回 `"foo"`（计数=1）
/// - 再次冲突自动递增生成 `"foo2"`、`"foo3"`... 且跳过已占用名称
/// - 支持单级父作用域只读继承检查（`parent` 指针）
pub struct NumberScopeRenamer {
    entries: [MAX_SCOPE_NAMES]ScopeNameEntry = [ScopeNameEntry{}; MAX_SCOPE_NAMES],
    len: usize = 0,
    parent: (*const NumberScopeRenamer)? = null,

    static fn init(): NumberScopeRenamer {
        return NumberScopeRenamer{
            entries: [ScopeNameEntry{}; MAX_SCOPE_NAMES],
            len: 0,
            parent: null,
        };
    }

    static fn initWithParent(parent: *const NumberScopeRenamer): NumberScopeRenamer {
        return NumberScopeRenamer{
            entries: [ScopeNameEntry{}; MAX_SCOPE_NAMES],
            len: 0,
            parent: parent,
        };
    }

    fn findLocalIndex(name: string): usize? {
        let i: usize = 0;
        while (i < _.len) {
            if (_.entries[i].name.eql(name)) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    fn isUsedInChain(name: string): bool {
        if (_.findLocalIndex(name) != null) {
            return true;
        }
        if (p := _.parent) {
            return p.isUsedInChain(name);
        }
        return false;
    }

    fn reserve(*_, name: string): ! {
        if (_.findLocalIndex(name) != null) {
            return;
        }
        if (_.len >= MAX_SCOPE_NAMES) {
            return err.ScopeCapacityExceeded;
        }
        const iname = InlineName.fromSlice(name)?;
        _.entries[_.len] = ScopeNameEntry{ name: iname, count: 1 };
        _.len += 1;
    }

    /// 分配不冲突的合法标识符并记录到当前作用域（输出写入 `out`）
    fn findUnusedName(*_, raw_name: string, out: []u8): string! {
        let clean_buf: [INLINE_NAME_CAP]u8 = [0; INLINE_NAME_CAP];
        const prefix = ensureValidJsIdentifier(raw_name, clean_buf[0..]) catch return err.NameTooLong;

        if (!_.isUsedInChain(prefix) && !isJsReservedWord(prefix)) {
            if (out.len < prefix.len) {
                return err.BufferTooSmall;
            }
            if (prefix.len > 0) {
                @memcpy(out[0..prefix.len], prefix);
            }
            _.reserve(prefix)?;
            return out[0..prefix.len];
        }

        // Determine starting suffix number: same-scope count + 1, or 2 if parent/keyword collision
        let tries: u32 = 2;
        const local_idx = _.findLocalIndex(prefix);
        if (idx := local_idx) {
            tries = _.entries[idx].count + 1;
        }

        let cand_buf: [INLINE_NAME_CAP]u8 = [0; INLINE_NAME_CAP];
        while (true) {
            if (prefix.len >= INLINE_NAME_CAP) {
                return err.NameTooLong;
            }
            if (prefix.len > 0) {
                @memcpy(cand_buf[0..prefix.len], prefix);
            }
            const num_len = appendU32Decimal(tries, cand_buf[prefix.len..]) catch return err.NameTooLong;
            const total_len = prefix.len + num_len;
            const cand = cand_buf[0..total_len];

            if (!_.isUsedInChain(cand) && !isJsReservedWord(cand)) {
                if (idx := local_idx) {
                    _.entries[idx].count = tries;
                }
                _.reserve(cand)?;
                if (out.len < total_len) {
                    return err.BufferTooSmall;
                }
                let k: usize = 0;
                while (k < total_len) {
                    out[k] = cand[k];
                    k += 1;
                }
                return out[0..total_len];
            }
            tries += 1;
        }
    }
}

/// 待最小化分配的符号槽位（对齐 Bun `SymbolSlot`）
pub struct MinifySymbolSlot {
    symbol_id: u32 = 0,
    count: u32 = 0,
    needs_capital_for_jsx: bool = false,
    assigned: InlineName = InlineName.empty(),
}

/// 按引用频率排序的最小化短标识符分配器（对齐 Bun `MinifyRenamer.assignNamesByFrequency`）
pub struct MinifySlotAssigner {
    slots: [MAX_MINIFY_SLOTS]MinifySymbolSlot = [MinifySymbolSlot{}; MAX_MINIFY_SLOTS],
    slot_len: usize = 0,
    reserved: [MAX_RESERVED_NAMES]InlineName = [InlineName.empty(); MAX_RESERVED_NAMES],
    reserved_len: usize = 0,

    static fn init(): MinifySlotAssigner {
        return MinifySlotAssigner{
            slots: [MinifySymbolSlot{}; MAX_MINIFY_SLOTS],
            slot_len: 0,
            reserved: [InlineName.empty(); MAX_RESERVED_NAMES],
            reserved_len: 0,
        };
    }

    fn addReserved(*_, name: string): ! {
        if (_.isReserved(name)) {
            return;
        }
        if (_.reserved_len >= MAX_RESERVED_NAMES) {
            return err.ReservedCapacityExceeded;
        }
        _.reserved[_.reserved_len] = InlineName.fromSlice(name)?;
        _.reserved_len += 1;
    }

    fn isReserved(name: string): bool {
        let i: usize = 0;
        while (i < _.reserved_len) {
            if (_.reserved[i].eql(name)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn addSlot(*_, symbol_id: u32, count: u32, needs_capital_for_jsx: bool): usize! {
        if (_.slot_len >= MAX_MINIFY_SLOTS) {
            return err.SlotCapacityExceeded;
        }
        const idx = _.slot_len;
        _.slots[idx] = MinifySymbolSlot{
            symbol_id: symbol_id,
            count: count,
            needs_capital_for_jsx: needs_capital_for_jsx,
            assigned: InlineName.empty(),
        };
        _.slot_len += 1;
        return idx;
    }

    /// 按引用频率降序（平局按原始 slot 顺序升序）分配最短合法标识符
    fn assignByFrequency(*_, ns: SlotNamespace): ! {
        // Build index permutation sorted by (count desc, original_idx asc) via insertion sort (stable, zero alloc)
        let order: [MAX_MINIFY_SLOTS]usize = undefined;
        let i: usize = 0;
        while (i < _.slot_len) {
            order[i] = i;
            i += 1;
        }

        i = 1;
        while (i < _.slot_len) {
            const key_idx = order[i];
            const key_count = _.slots[key_idx].count;
            let j = i;
            while (j > 0) {
                const prev_idx = order[j - 1];
                const prev_count = _.slots[prev_idx].count;
                if (prev_count < key_count) {
                    order[j] = prev_idx;
                    j -= 1;
                } else {
                    break;
                }
            }
            order[j] = key_idx;
            i += 1;
        }

        let next_idx: u64 = 0;
        let name_buf: [INLINE_NAME_CAP]u8 = [0; INLINE_NAME_CAP];

        i = 0;
        while (i < _.slot_len) {
            const slot_idx = order[i];
            const need_cap = _.slots[slot_idx].needs_capital_for_jsx;

            while (true) {
                const cand = encodeMinifiedName(next_idx, name_buf[0..])?;
                next_idx += 1;

                if (ns == .default) {
                    if (isJsReservedWord(cand) || _.isReserved(cand)) {
                        continue;
                    }
                    if (need_cap && cand[0] >= 'a' && cand[0] <= 'z') {
                        continue;
                    }
                } else if (ns == .label) {
                    if (isJsReservedWord(cand)) {
                        continue;
                    }
                }

                _.slots[slot_idx].assigned = InlineName.fromSlice(cand)?;
                break;
            }
            i += 1;
        }
    }

    fn assignedName(slot_idx: usize): string? {
        if (slot_idx >= _.slot_len) {
            return null;
        }
        return _.slots[slot_idx].assigned.slice();
    }
}
