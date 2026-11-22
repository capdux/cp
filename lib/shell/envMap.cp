// Source: Absorbed from /data/dev/bun/src/shell/EnvMap.cp | Synced: 2026-11-22T14:32:00Z
// Pure-CP zero-allocation Shell Reference-Counted Environment Map (`ShellEnvMap`).
// Aligned with Bun `src/shell/EnvMap.cp` and `docs/runtime/shell.mdx`:
// - Stores `(EnvStr, EnvStr)` key-value pairs backed by `EnvStrPool`.
// - Supports both POSIX case-sensitive and Windows ASCII case-insensitive key matching
//   (`PATH` == `Path` == `path`).
// - Preserves Bun's exact refcount ownership semantics:
//   * `insert()` refs new keys and new values, and derefs replaced values.
//   * `get()` returns a ref-incremented handle to the stored value.
//   * `clone()` copies entries and refs all keys/values for zero-copy subshell isolation.
//   * `clear()` / `remove()` deref all removed keys and values cleanly.
// - Provides amortized `memoryCost()` accounting and lexicographically sorted `KEY=VALUE\n` export formatting.

import { EnvStr, EnvStrPool } from "./envStr.cp";

/// 单个 Shell 环境表支持的最大环境变量条目数
pub const ENV_MAP_MAX_ENTRIES: usize = 64;

/// 环境表错误集
pub err EnvMapError {
    MapFull,
    EmptyKey,
    BufferTooSmall,
}

/// 单条环境变量键值记录
pub struct EnvMapEntry {
    key: EnvStr = EnvStr{},
    val: EnvStr = EnvStr{},
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn keysEqual(a: []const u8, b: []const u8, case_insensitive: bool): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (case_insensitive) {
            if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        } else {
            if (a[i] != b[i]) return false;
        }
    }
    return true;
}

fn bytesLessThan(a: []const u8, b: []const u8): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    for (i of 0..min_len) {
        if (a[i] != b[i]) return a[i] < b[i];
    }
    return a.len < b.len;
}

fn copyInto(dst: []u8, offset: usize, src: []const u8): usize! {
    if (offset + src.len > dst.len) return err.BufferTooSmall;
    for (i of 0..src.len) {
        dst[offset + i] = src[i];
    }
    return offset + src.len;
}

fn copyByte(dst: []u8, offset: usize, b: u8): usize! {
    if (offset + 1 > dst.len) return err.BufferTooSmall;
    dst[offset] = b;
    return offset + 1;
}

/// 零堆分配引用计数 Shell 环境变量映射表（对齐 Bun `EnvMap`）
pub struct ShellEnvMap {
    entries: [ENV_MAP_MAX_ENTRIES]EnvMapEntry = [EnvMapEntry{}; ENV_MAP_MAX_ENTRIES],
    count: usize = 0,
    /// `false` = POSIX 大小写敏感，`true` = Windows ASCII 大小写不敏感
    case_insensitive_keys: bool = false,

    static fn init(case_insensitive_keys: bool): ShellEnvMap {
        return ShellEnvMap{
            case_insensitive_keys: case_insensitive_keys,
        };
    }

    /// 返回当前条目数量
    fn len(): usize {
        return _.count;
    }

    /// 返回当前条目切片
    fn slice(): []const EnvMapEntry {
        return _.entries[0.._.count];
    }

    fn findIndex(pool: *const EnvStrPool, key_bytes: []const u8): (usize?)! {
        for (i of 0.._.count) {
            const k = pool.slice(_.entries[i].key)?;
            if (keysEqual(k, key_bytes, _.case_insensitive_keys)) {
                return i;
            }
        }
        return null;
    }

    /// 插入或更新环境变量：新键递增 key/val 引用计数；更新已有键时先递减旧值引用计数再递增新值引用计数
    fn insert(*_, pool: *EnvStrPool, key: EnvStr, val: EnvStr): bool! {
        if (key.isEmpty()) return err.EmptyKey;
        const key_bytes = pool.slice(key)?;
        if (idx := _.findIndex(pool, key_bytes)?) {
            * = pool.ref(val)?;
            * = pool.deref(_.entries[idx].val)?;
            _.entries[idx].val = val;
            return false; // updated existing
        }
        if (_.count >= ENV_MAP_MAX_ENTRIES) {
            return err.MapFull;
        }
        * = pool.ref(key)?;
        * = pool.ref(val)?;
        _.entries[_.count] = EnvMapEntry{
            key: key,
            val: val,
        };
        _.count += 1;
        return true; // inserted new
    }

    /// 查找环境变量，命中时递增返回值引用计数并返回（调用方使用完毕需 `pool.deref()`）
    fn get(pool: *EnvStrPool, key: EnvStr): (EnvStr?)! {
        if (key.isEmpty()) return null;
        const key_bytes = pool.slice(key)?;
        if (idx := _.findIndex(pool, key_bytes)?) {
            const v = _.entries[idx].val;
            * = pool.ref(v)?;
            return v;
        }
        return null;
    }

    /// 只读查询环境变量字节值（不修改引用计数）
    fn getSlice(pool: *const EnvStrPool, key_bytes: []const u8): (([]const u8)?)! {
        if (key_bytes.len == 0) return null;
        if (idx := _.findIndex(pool, key_bytes)?) {
            return pool.slice(_.entries[idx].val)?;
        }
        return null;
    }

    /// 删除指定键并递减其 key/val 引用计数（返回是否找到并删除）
    fn remove(*_, pool: *EnvStrPool, key_bytes: []const u8): bool! {
        if (key_bytes.len == 0) return false;
        if (idx := _.findIndex(pool, key_bytes)?) {
            * = pool.deref(_.entries[idx].key)?;
            * = pool.deref(_.entries[idx].val)?;
            let i: usize = idx;
            while (i + 1 < _.count) {
                _.entries[i] = _.entries[i + 1];
                i += 1;
            }
            _.count -= 1;
            return true;
        }
        return false;
    }

    /// 清空全部条目并递减所有托管 key/val 引用计数（对齐 `clearRetainingCapacity`）
    fn clear(*_, pool: *EnvStrPool): usize! {
        const prev = _.count;
        for (i of 0.._.count) {
            * = pool.deref(_.entries[i].key)?;
            * = pool.deref(_.entries[i].val)?;
        }
        _.count = 0;
        return prev;
    }

    /// 零字节拷贝克隆环境表（用于子 Shell / 管道隔离，批量递增所有 key/val 引用计数）
    fn clone(pool: *EnvStrPool): ShellEnvMap! {
        let out = ShellEnvMap.init(_.case_insensitive_keys);
        out.count = _.count;
        for (i of 0.._.count) {
            out.entries[i] = _.entries[i];
            * = pool.ref(_.entries[i].key)?;
            * = pool.ref(_.entries[i].val)?;
        }
        return out;
    }

    /// 计算环境表均摊总内存开销（对齐 Bun `EnvMap.memoryCost`）
    fn memoryCost(pool: *const EnvStrPool): usize {
        let total: usize = 64;
        total += _.count * (32 as usize);
        for (i of 0.._.count) {
            total += pool.memoryCost(_.entries[i].key);
            total += pool.memoryCost(_.entries[i].val);
        }
        return total;
    }

    /// 按键名字典序升序格式化全部导出条目为 `KEY=VALUE\n`（对齐 Bun `export` 无参输出）
    fn formatSortedExports(pool: *const EnvStrPool, out: []u8): usize! {
        let order: [ENV_MAP_MAX_ENTRIES]usize = [0; ENV_MAP_MAX_ENTRIES];
        for (i of 0.._.count) {
            order[i] = i;
        }
        // 稳定插入排序（零堆分配）
        for (i of 1.._.count) {
            const cur_idx = order[i];
            const cur_key = pool.slice(_.entries[cur_idx].key)?;
            let j = i;
            while (j > 0) {
                const prev_key = pool.slice(_.entries[order[j - 1]].key)?;
                if (!bytesLessThan(cur_key, prev_key)) break;
                order[j] = order[j - 1];
                j -= 1;
            }
            order[j] = cur_idx;
        }

        let pos: usize = 0;
        for (i of 0.._.count) {
            const ent = _.entries[order[i]];
            const k = pool.slice(ent.key)?;
            const v = pool.slice(ent.val)?;
            pos = copyInto(out, pos, k)?;
            pos = copyByte(out, pos, '=')?;
            pos = copyInto(out, pos, v)?;
            pos = copyByte(out, pos, '\n')?;
        }
        return pos;
    }
}
