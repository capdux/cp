// Source: Absorbed from /data/dev/bun/src/collections/StaticHashMap.cp | Synced: 2026-11-24T07:05:00Z

pub const EMPTY_HASH: u64 = 0xFFFFFFFFFFFFFFFF;

pub err Error {
    MapFull,
}

/// FNV-1a 64-bit 字节切片哈希（保证结果永不为 EMPTY_HASH）
pub fn fnv1a64(bytes: []const u8): u64 {
    let h: u64 = 0xcbf29ce484222325;
    let i: usize = 0;
    while (i < bytes.len) {
        h ^= bytes[i] as u64;
        h *%= 0x100000001b3;
        i += 1;
    }
    return if (h == EMPTY_HASH) (EMPTY_HASH - 1) else h;
}

/// ASCII 大小写无关 FNV-1a 64-bit 哈希
pub fn fnv1a64AsciiLower(bytes: []const u8): u64 {
    let h: u64 = 0xcbf29ce484222325;
    let i: usize = 0;
    while (i < bytes.len) {
        let b = bytes[i];
        if (b >= 'A' && b <= 'Z') {
            b += 32;
        }
        h ^= b as u64;
        h *%= 0x100000001b3;
        i += 1;
    }
    return if (h == EMPTY_HASH) (EMPTY_HASH - 1) else h;
}

/// SplitMix64 整型位混合哈希（保证结果永不为 EMPTY_HASH）
pub fn splitmix64(x_in: u64): u64 {
    let x = x_in +% 0x9e3779b97f4a7c15;
    x = (x ^ (x >> 30)) *% 0xbf58476d1ce4e5b9;
    x = (x ^ (x >> 27)) *% 0x94d049bb133111eb;
    x = x ^ (x >> 31);
    return if (x == EMPTY_HASH) (EMPTY_HASH - 1) else x;
}

fn asciiEqlIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') { ca += 32; }
        if (cb >= 'A' && cb <= 'Z') { cb += 32; }
        if (ca != cb) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 精确匹配字符串哈希上下文
pub struct StringContext {
    fn hash(k: string): u64 {
        return fnv1a64(k);
    }

    fn eql(a: string, b: string): bool {
        return std.mem.eql(u8, a, b);
    }
}

/// ASCII 大小写无关字符串哈希上下文
pub struct AsciiIgnoreCaseContext {
    fn hash(k: string): u64 {
        return fnv1a64AsciiLower(k);
    }

    fn eql(a: string, b: string): bool {
        return asciiEqlIgnoreCase(a, b);
    }
}

/// 整型键哈希上下文（支持任意整型 u8..u64 / i8..i64 / usize）
pub struct IntContext<K> {
    fn hash(k: K): u64 {
        const raw: u64 = @as(u64, @intCast(k));
        return splitmix64(raw);
    }

    fn eql(a: K, b: K): bool {
        return a == b;
    }
}

/// 静态哈希表键值对构造项
pub struct KeyValue<K, V> {
    key: K,
    value: V,
}

/// 内部槽位条目
pub struct Slot<K, V> {
    hash: u64 = EMPTY_HASH,
    key: K = std.mem.zeroes(K),
    value: V = std.mem.zeroes(V),

    fn isEmpty(): bool {
        return _.hash == EMPTY_HASH;
    }
}

/// 固定容量、零堆分配静态哈希表（支持编译期 `const` 构建与运行期 O(1) 开放寻址查找）
/// `CAP` 必须为 >= 2 的 2 的幂次
pub struct StaticHashMap<K, V, Context, const CAP: usize> {
    entries: [CAP]Slot<K, V> = [Slot<K, V>{}; CAP],
    len: usize = 0,
    ctx: Context = std.mem.zeroes(Context),

    static fn init(): StaticHashMap<K, V, Context, CAP> {
        if (CAP < 2 || (CAP & (CAP - 1)) != 0) {
            @compileError("StaticHashMap capacity CAP must be a power of two >= 2");
        }
        return StaticHashMap<K, V, Context, CAP>{};
    }

    static fn initWithContext(context: Context): StaticHashMap<K, V, Context, CAP> {
        if (CAP < 2 || (CAP & (CAP - 1)) != 0) {
            @compileError("StaticHashMap capacity CAP must be a power of two >= 2");
        }
        return StaticHashMap<K, V, Context, CAP>{ ctx: context };
    }

    /// 编译期或运行期从键值对切片一键构建静态哈希表
    static fn fromPairs(pairs: []const KeyValue<K, V>): StaticHashMap<K, V, Context, CAP> {
        let m = StaticHashMap<K, V, Context, CAP>.init();
        let i: usize = 0;
        while (i < pairs.len) {
            * = m.put(pairs[i].key, pairs[i].value);
            i += 1;
        }
        return m;
    }

    /// 返回当前已存入元素数量
    fn count(): usize {
        return _.len;
    }

    /// 返回表总槽位容量
    fn capacity(): usize {
        return CAP;
    }

    /// 返回负载因子千分比（0..=1000）
    fn loadFactorPermille(): u32 {
        return ((_.len * 1000) / CAP) as u32;
    }

    /// 清空表内容并保留定长容量
    fn clear(*_) {
        let i: usize = 0;
        while (i < CAP) {
            _.entries[i] = Slot<K, V>{};
            i += 1;
        }
        _.len = 0;
    }

    /// 插入或更新键值对；若表满且为新键返回 false
    fn put(*_, key: K, value: V): bool {
        let h = _.ctx.hash(key);
        if (h == EMPTY_HASH) {
            h = EMPTY_HASH - 1;
        }
        const mask: usize = CAP - 1;
        let idx: usize = (h as usize) & mask;
        let step: usize = 0;

        while (step < CAP) {
            if (_.entries[idx].hash == EMPTY_HASH) {
                _.entries[idx] = Slot<K, V>{
                    hash: h,
                    key: key,
                    value: value,
                };
                _.len += 1;
                return true;
            }
            if (_.entries[idx].hash == h && _.ctx.eql(_.entries[idx].key, key)) {
                _.entries[idx].value = value;
                return true;
            }
            idx = (idx + 1) & mask;
            step += 1;
        }
        return false;
    }

    /// 插入或更新键值对；表满时返回 err.MapFull
    fn putChecked(*_, key: K, value: V)! {
        if (!_.put(key, value)) {
            return err.MapFull;
        }
    }

    /// O(1) 查找键对应的值；不存在返回 null
    fn get(key: K): V? {
        let h = _.ctx.hash(key);
        if (h == EMPTY_HASH) {
            h = EMPTY_HASH - 1;
        }
        const mask: usize = CAP - 1;
        let idx: usize = (h as usize) & mask;
        let step: usize = 0;

        while (step < CAP) {
            const slot = _.entries[idx];
            if (slot.hash == EMPTY_HASH) {
                return null;
            }
            if (slot.hash == h && _.ctx.eql(slot.key, key)) {
                return slot.value;
            }
            idx = (idx + 1) & mask;
            step += 1;
        }
        return null;
    }

    /// O(1) 判断键是否存在
    fn has(key: K): bool {
        return _.get(key) != null;
    }

    /// 统计查找指定键所需的探测步数（命中或遇空槽停止）
    fn probeCount(key: K): usize {
        let h = _.ctx.hash(key);
        if (h == EMPTY_HASH) {
            h = EMPTY_HASH - 1;
        }
        const mask: usize = CAP - 1;
        let idx: usize = (h as usize) & mask;
        let step: usize = 0;

        while (step < CAP) {
            step += 1;
            const slot = _.entries[idx];
            if (slot.hash == EMPTY_HASH) {
                return step;
            }
            if (slot.hash == h && _.ctx.eql(slot.key, key)) {
                return step;
            }
            idx = (idx + 1) & mask;
        }
        return step;
    }

    /// 删除指定键（采用后向移位修复探测链，不产生 tombstone），返回被删除的值
    fn delete(*_, key: K): V? {
        let h = _.ctx.hash(key);
        if (h == EMPTY_HASH) {
            h = EMPTY_HASH - 1;
        }
        const mask: usize = CAP - 1;
        let idx: usize = (h as usize) & mask;
        let step: usize = 0;

        while (step < CAP) {
            if (_.entries[idx].hash == EMPTY_HASH) {
                return null;
            }
            if (_.entries[idx].hash == h && _.ctx.eql(_.entries[idx].key, key)) {
                const removed = _.entries[idx].value;
                let hole = idx;
                let cur = (hole + 1) & mask;
                let shift_steps: usize = 0;
                while (shift_steps < CAP) {
                    if (_.entries[cur].hash == EMPTY_HASH) {
                        break;
                    }
                    const home: usize = (_.entries[cur].hash as usize) & mask;
                    const dist_cur = (cur -% home) & mask;
                    const dist_hole = (cur -% hole) & mask;
                    if (dist_cur >= dist_hole) {
                        _.entries[hole] = _.entries[cur];
                        hole = cur;
                    }
                    cur = (cur + 1) & mask;
                    shift_steps += 1;
                }
                _.entries[hole] = Slot<K, V>{};
                _.len -= 1;
                return removed;
            }
            idx = (idx + 1) & mask;
            step += 1;
        }
        return null;
    }
}
