# `cap:static_hashmap` — 编译期/零分配静态哈希表

> 参考来源：/data/dev/bun/src/collections/StaticHashMap.cp | 吸收日期：2026-11-22

`cap:static_hashmap` 提供固定 2 的幂次容量、零堆分配、开放寻址的静态哈希表 `StaticHashMap<K, V, Context, const CAP: usize>`：

- 支持在编译期通过 `fromPairs(...)` 直接构建 `const` 只读查找表，运行期零分配 O(1) 查询；
- 内置纯 CP `fnv1a64`、`fnv1a64AsciiLower`、`splitmix64` 哈希函数及 `StringContext`、`AsciiIgnoreCaseContext`、`IntContext<K>`；
- 删除采用后向移位修复探测链算法（Backward-Shift Deletion），不产生 tombstone 退化，并支持 `probeCount` 探测步数统计与满表 `MapFull` 安全防护。

---

## 快速导入

```cp
import {
    StaticHashMap,
    KeyValue,
    StringContext,
    AsciiIgnoreCaseContext,
    IntContext,
    Error,
    EMPTY_HASH,
    fnv1a64,
    fnv1a64AsciiLower,
    splitmix64,
} from "cap:static_hashmap";
```

---

## API 参考

### 1. 内置哈希函数与上下文

| 符号 | 说明 |
| :--- | :--- |
| `fnv1a64(bytes: []const u8): u64` | FNV-1a 64-bit 哈希（永不返回 `EMPTY_HASH`） |
| `fnv1a64AsciiLower(bytes: []const u8): u64` | ASCII 大小写无关 FNV-1a 64-bit 哈希 |
| `splitmix64(x: u64): u64` | SplitMix64 64-bit 整数混合哈希 |
| `StringContext` | 精确匹配 `string` 键上下文 |
| `AsciiIgnoreCaseContext` | ASCII 大小写无关 `string` 键上下文 |
| `IntContext<K>` | 整型键上下文 |

### 2. `StaticHashMap<K, V, Context, const CAP: usize>`

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `init` / `initWithContext` | `static fn init(): Self` | 构造空表（编译期断言 `CAP` 为 `>= 2` 的 2 的幂） |
| `fromPairs` | `static fn fromPairs(pairs: []const KeyValue<K, V>): Self` | 从键值对切片一键构建静态表（支持 `const` 编译期求值） |
| `put` / `putChecked` | `fn put(*_, key, value): bool` / `fn putChecked(*_, key, value)!` | 插入或更新；满表返回 `false` / `err.MapFull` |
| `get` / `has` | `fn get(key): V?` / `fn has(key): bool` | O(1) 查找与存在性检测 |
| `delete` | `fn delete(*_, key): V?` | 后向移位删除并返回旧值 |
| `clear` | `fn clear(*_)` | 清空全部槽位 |
| `count` / `capacity` / `loadFactorPermille` | `fn ...(): usize / u32` | 元素数、总槽位数与千分比负载因子 |
| `probeCount` | `fn probeCount(key): usize` | 返回查找指定键的探测步数 |

---

## 使用示例

```cp
import { StaticHashMap, KeyValue, StringContext } from "cap:static_hashmap";

const METHOD_MAP = StaticHashMap<string, u16, StringContext, 16>.fromPairs([
    KeyValue<string, u16>{ key: "GET", value: 1 },
    KeyValue<string, u16>{ key: "POST", value: 2 },
    KeyValue<string, u16>{ key: "PUT", value: 3 },
    KeyValue<string, u16>{ key: "DELETE", value: 4 },
]);

fn lookupMethod(name: string): u16? {
    return METHOD_MAP.get(name);
}
```

## 上游行为契约与性能/边界特性

1. **无 Tombstone 退化（Backward-Shift Deletion）**：
   - 与传统开放寻址表不同，`delete` 通过后向移位修复后续探测链，长期高频增删后仍保持紧凑探测距离，不会因墓碑堆积退化为 $O(N)$，可通过 `probeCount(key)` 实时观测探测步数。
2. **容量与负载因子契约**：
   - 模板参数 `CAP` 必须为 `>= 2` 的 2 的幂（利用位掩码 `hash & (CAP - 1)` 替代取模运算）；为维持 $O(1)$ 低冲突率，建议保持 `loadFactorPermille() <= 700`（负载因子 $\le 0.7$）。
3. **哨兵值与满表保护**：
   - `EMPTY_HASH`（`0xFFFFFFFFFFFFFFFF`）保留为空槽哨兵，内置 `fnv1a64` / `fnv1a64AsciiLower` / `splitmix64` 均已自动避开该值；自定义 `Context` 哈希函数亦须避免返回 `EMPTY_HASH`；满表写入时 `put` 返回 `false`、`putChecked` 返回 `Error.MapFull`。
