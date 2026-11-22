// Source: Absorbed from /data/dev/bun/src/test_runner/expect/toBeArrayOfSize.cp | Synced: 2026-11-22T12:28:01Z
// Pure-CP zero-allocation sequence/slice/collection expect matchers
// (toBeEmpty, toHaveLength, toBeArrayOfSize, toContain, toIncludeAllMembers, toIncludeAnyMembers, sorted/unique).

fn eqBytes(a: []const u8, b: []const u8): bool {
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

/// 判定字节序列/字符串是否为空（对齐 Bun `toBeEmpty`）
pub fn toBeEmptyBytes(data: []const u8): bool {
    return data.len == 0;
}

/// 判定字符串切片是否为空
pub fn toBeEmptyStrSlice(items: []const []const u8): bool {
    return items.len == 0;
}

/// 判定整数切片是否为空
pub fn toBeEmptyI64Slice(items: []const i64): bool {
    return items.len == 0;
}

/// 判定字节序列长度是否精确等于 `expected`（对齐 Bun `toHaveLength`）
pub fn toHaveLengthBytes(data: []const u8, expected: usize): bool {
    return data.len == expected;
}

/// 判定字符串切片元素数量是否精确等于 `expected`（对齐 Bun `toBeArrayOfSize`）
pub fn toBeArrayOfSizeStr(items: []const []const u8, expected: usize): bool {
    return items.len == expected;
}

/// 判定整数切片元素数量是否精确等于 `expected`（对齐 Bun `toBeArrayOfSize`）
pub fn toBeArrayOfSizeI64(items: []const i64, expected: usize): bool {
    return items.len == expected;
}

/// 判定字符串切片是否包含指定成员（对齐 Bun `toContain`）
pub fn toContainStr(items: []const []const u8, needle: []const u8): bool {
    let i: usize = 0;
    while (i < items.len) {
        if (eqBytes(items[i], needle)) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// 判定整数切片是否包含指定成员（对齐 Bun `toContain`）
pub fn toContainI64(items: []const i64, needle: i64): bool {
    let i: usize = 0;
    while (i < items.len) {
        if (items[i] == needle) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn countStr(items: []const []const u8, target: []const u8): usize {
    let c: usize = 0;
    let i: usize = 0;
    while (i < items.len) {
        if (eqBytes(items[i], target)) {
            c += 1;
        }
        i += 1;
    }
    return c;
}

fn countI64(items: []const i64, target: i64): usize {
    let c: usize = 0;
    let i: usize = 0;
    while (i < items.len) {
        if (items[i] == target) {
            c += 1;
        }
        i += 1;
    }
    return c;
}

/// 顺序无关的多重集全等判定（字符串切片，精确比对每个成员的出现频次）
pub fn toIncludeAllMembersStr(actual: []const []const u8, expected: []const []const u8): bool {
    if (actual.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < expected.len) {
        const target = expected[i];
        if (countStr(actual, target) != countStr(expected, target)) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 顺序无关的多重集全等判定（整数切片，精确比对每个成员的出现频次）
pub fn toIncludeAllMembersI64(actual: []const i64, expected: []const i64): bool {
    if (actual.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < expected.len) {
        const target = expected[i];
        if (countI64(actual, target) != countI64(expected, target)) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 判定字符串切片是否至少包含候选集中的任一成员
pub fn toIncludeAnyMembersStr(actual: []const []const u8, candidates: []const []const u8): bool {
    let i: usize = 0;
    while (i < candidates.len) {
        if (toContainStr(actual, candidates[i])) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// 判定整数切片是否至少包含候选集中的任一成员
pub fn toIncludeAnyMembersI64(actual: []const i64, candidates: []const i64): bool {
    let i: usize = 0;
    while (i < candidates.len) {
        if (toContainI64(actual, candidates[i])) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// 严格按顺序逐项相等判定（字符串切片）
pub fn toIncludeSameOrderStr(actual: []const []const u8, expected: []const []const u8): bool {
    if (actual.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < actual.len) {
        if (!eqBytes(actual[i], expected[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 严格按顺序逐项相等判定（整数切片）
pub fn toIncludeSameOrderI64(actual: []const i64, expected: []const i64): bool {
    if (actual.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < actual.len) {
        if (actual[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 判定整数切片是否按非递减（升序）排列
pub fn toBeSortedAscI64(items: []const i64): bool {
    if (items.len <= 1) {
        return true;
    }
    let i: usize = 1;
    while (i < items.len) {
        if (items[i] < items[i - 1]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 判定整数切片是否按非递增（降序）排列
pub fn toBeSortedDescI64(items: []const i64): bool {
    if (items.len <= 1) {
        return true;
    }
    let i: usize = 1;
    while (i < items.len) {
        if (items[i] > items[i - 1]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 判定整数切片中全部元素互不重复
pub fn toBeUniqueI64(items: []const i64): bool {
    let i: usize = 0;
    while (i < items.len) {
        let j: usize = i + 1;
        while (j < items.len) {
            if (items[i] == items[j]) {
                return false;
            }
            j += 1;
        }
        i += 1;
    }
    return true;
}

/// 判定字符串切片中全部元素互不重复
pub fn toBeUniqueStr(items: []const []const u8): bool {
    let i: usize = 0;
    while (i < items.len) {
        let j: usize = i + 1;
        while (j < items.len) {
            if (eqBytes(items[i], items[j])) {
                return false;
            }
            j += 1;
        }
        i += 1;
    }
    return true;
}

/// 带单次 `.not` 反转与断言统计的零分配序列断言匹配器
pub struct ExpectSeqMatcher {
    negated: bool = false,
    assertions: usize = 0,
    passed: usize = 0,
    failed: usize = 0,

    static fn init(): ExpectSeqMatcher {
        return ExpectSeqMatcher{};
    }

    fn negate(*_) {
        _.negated = !_.negated;
    }

    fn record(*_, raw_pass: bool): bool {
        const ok = if (_.negated) !raw_pass else raw_pass;
        _.negated = false;
        _.assertions += 1;
        if (ok) {
            _.passed += 1;
        } else {
            _.failed += 1;
        }
        return ok;
    }

    fn checkEmptyBytes(*_, data: []const u8): bool {
        return _.record(toBeEmptyBytes(data));
    }

    fn checkEmptyStr(*_, items: []const []const u8): bool {
        return _.record(toBeEmptyStrSlice(items));
    }

    fn checkEmptyI64(*_, items: []const i64): bool {
        return _.record(toBeEmptyI64Slice(items));
    }

    fn checkArrayOfSizeStr(*_, items: []const []const u8, expected: usize): bool {
        return _.record(toBeArrayOfSizeStr(items, expected));
    }

    fn checkArrayOfSizeI64(*_, items: []const i64, expected: usize): bool {
        return _.record(toBeArrayOfSizeI64(items, expected));
    }

    fn checkContainStr(*_, items: []const []const u8, needle: []const u8): bool {
        return _.record(toContainStr(items, needle));
    }

    fn checkContainI64(*_, items: []const i64, needle: i64): bool {
        return _.record(toContainI64(items, needle));
    }

    fn checkIncludeAllMembersStr(*_, actual: []const []const u8, expected: []const []const u8): bool {
        return _.record(toIncludeAllMembersStr(actual, expected));
    }

    fn checkIncludeAllMembersI64(*_, actual: []const i64, expected: []const i64): bool {
        return _.record(toIncludeAllMembersI64(actual, expected));
    }

    fn checkIncludeAnyMembersStr(*_, actual: []const []const u8, candidates: []const []const u8): bool {
        return _.record(toIncludeAnyMembersStr(actual, candidates));
    }

    fn checkIncludeAnyMembersI64(*_, actual: []const i64, candidates: []const i64): bool {
        return _.record(toIncludeAnyMembersI64(actual, candidates));
    }

    fn checkSortedAscI64(*_, items: []const i64): bool {
        return _.record(toBeSortedAscI64(items));
    }

    fn checkUniqueStr(*_, items: []const []const u8): bool {
        return _.record(toBeUniqueStr(items));
    }
}
