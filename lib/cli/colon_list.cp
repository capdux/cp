// Source: Absorbed from /data/dev/bun/src/cli/colon_list_type.cp | Synced: 2026-11-22T03:50:35Z
// Pure-CP zero-allocation CLI `key:value` / `key=value` colon-list parser:
// - Prefers whichever separator (`:` or `=`) appears first, matching Bun & esbuild CLI flags
//   (e.g. `--define FOO:1`, `--define FOO=bar:baz`, `--loader .svg=text`)
// - Optional strict non-empty key & leading-dot file extension validation (`parseLoaderMapping`)
// - Zero-copy batch parser into caller-supplied slice + last-wins lookup (`findColonValue`).

/// CLI 冒号/等号键值列表解析错误集
pub err ColonListError {
    InvalidSeparator,
    EmptyKey,
    InvalidExtensionPrefix,
    OutputBufferTooSmall,
}

/// 零拷贝 CLI 键值对（指向原始输入切片）
pub struct ColonPair {
    key: []const u8 = "",
    value: []const u8 = "",
    separator: u8 = ':',
}

fn bytesEql(a: []const u8, b: []const u8): bool {
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

/// 解析单条 `key:value` 或 `key=value` 映射（优先采用先出现的 `:` 或 `=` 作为分隔符，对齐 Bun `ColonListType`）
pub fn parseColonPair(raw: []const u8): ColonPair! {
    let i: usize = 0;
    while (i < raw.len) {
        const ch = raw[i];
        if (ch == ':' || ch == '=') {
            return ColonPair{
                key: raw[0..i],
                value: raw[i + 1..],
                separator: ch,
            };
        }
        i += 1;
    }
    return err.InvalidSeparator;
}

/// 解析单条键值映射并要求 `key` 非空
pub fn parseColonPairNonEmptyKey(raw: []const u8): ColonPair! {
    const pair = parseColonPair(raw)?;
    if (pair.key.len == 0) {
        return err.EmptyKey;
    }
    return pair;
}

/// 解析文件扩展名加载器映射（例如 `.ts:tsx` / `.svg=text`），要求扩展名非空且以 `.` 开头
pub fn parseLoaderMapping(raw: []const u8): ColonPair! {
    const pair = parseColonPairNonEmptyKey(raw)?;
    if (pair.key[0] != '.') {
        return err.InvalidExtensionPrefix;
    }
    return pair;
}

/// 零堆分配批量解析键值列表至调用方提供的 `out` 缓冲区，返回解析条数
pub fn parseColonList(inputs: []const ([]const u8), out: []ColonPair): usize! {
    if (out.len < inputs.len) {
        return err.OutputBufferTooSmall;
    }
    let i: usize = 0;
    while (i < inputs.len) {
        out[i] = parseColonPair(inputs[i])?;
        i += 1;
    }
    return inputs.len;
}

/// 零堆分配批量解析扩展名加载器映射列表至 `out` 缓冲区，返回解析条数
pub fn parseLoaderMappingList(inputs: []const ([]const u8), out: []ColonPair): usize! {
    if (out.len < inputs.len) {
        return err.OutputBufferTooSmall;
    }
    let i: usize = 0;
    while (i < inputs.len) {
        out[i] = parseLoaderMapping(inputs[i])?;
        i += 1;
    }
    return inputs.len;
}

/// 按 CLI 重复标志「后写覆盖（last-wins）」语义查找指定 `key` 对应的 `value`
pub fn findColonValue(pairs: []const ColonPair, key: []const u8): ([]const u8)? {
    let i = pairs.len;
    while (i > 0) {
        i -= 1;
        if (bytesEql(pairs[i].key, key)) {
            return pairs[i].value;
        }
    }
    return null;
}
