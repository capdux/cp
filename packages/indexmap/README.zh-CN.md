[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# indexmap

> **源包溯源**：吸收自 **crates.io** [`indexmap`](https://github.com/indexmap-rs/indexmap)（v2 / `MIT`），CP 官方洁净室实现 `indexmap@1.0.0`。

纯 CP 实现的零堆分配、保持插入顺序的哈希表。采用“紧凑顺序数组 + 开放寻址哈希索引表”双层结构，底层哈希复用 `@sindresorhus/fnv1a`。

## 核心亮点

- **零堆分配内联存储**：`IndexMapU64`（容量 64，128 哈希桶）与 `IndexMapStr`（容量 32，64 哈希桶，键长上限 64 字节）全程零堆分配。
- **零栈清零优化**：底层条目数组与临时键缓冲（`[64]IndexMapEntry`、`[32]IndexMapStrEntry`、`[64]u8`）均使用 `undefined` 初始化，大幅降低构造开销。
- **O(1) 查找 + 稳定顺序**：支持 O(1) 按键查找与按索引访问（`getIndex`、`first`、`last`、`pop`），同时提供 O(1) `swapRemove` 与保序 `shiftRemove`。

## 零成本迁移（Rust → CP）

直接沿用原包名 `indexmap`，无需改名：

**Rust：**
```rust
use indexmap::IndexMap;
let mut m = IndexMap::new();
m.insert("a", 1);
```

**CP：**
```typescript
import { IndexMapU64, IndexMapStr } from "indexmap";
```

## 实战示例

### 1. 字符串键保序哈希表（`IndexMapStr`）

```typescript
import { IndexMapStr } from "indexmap";

export fn demoStrMap(): i64 {
    let mut m = IndexMapStr.new();
    _ = m.insert("alpha", 10);
    _ = m.insert("beta", 20);
    _ = m.insert("gamma", 30);

    // 原位更新不改变已有顺序
    let upd = m.insert("beta", 200);
    // upd.replaced == true, upd.old_val == 20, upd.index == 1

    // shiftRemove 保持剩余键的插入顺序
    let removed = m.shiftRemove("alpha");
    if (removed.found) {
        return m.first().val; // 200 ("beta")
    }
    return 0;
}
```

### 2. 整数键哈希表（`IndexMapU64`）与顺序遍历

```typescript
import { IndexMapU64 } from "indexmap";

export fn demoU64Map(): i64 {
    let mut m = IndexMapU64.new();
    _ = m.insert(1001, 5);
    _ = m.insert(1002, 15);

    let mut total: i64 = 0;
    let mut i: usize = 0;
    while (i < m.len()) {
        let e = m.getIndex(i);
        if (e.found) {
            total += e.val;
        }
        i += 1;
    }
    return total; // 20
}
```

## 核心 API 速览

`IndexMapU64`（容量 64，键 `u64`）与 `IndexMapStr`（容量 32，键 `string` ≤ 64B）提供对称方法集：

- `new()`、`len()`、`capacity()`、`isEmpty()`、`clear()`
- `insert(key, val: i64): InsertResult`（`ok`、`replaced`、`old_val`、`index`）
- `get(key): GetValResult`（`found`、`val`）、`getIndexOf(key): i64`（不存在返回 `-1`）、`containsKey(key): bool`
- `getIndex(idx)` / `first()` / `last()` / `pop()`（字符串版返回 `GetStrEntryResult`，可用 `keyStr()` 获取键切片）
- `swapRemove(key)`（O(1)，将末尾元素换入空位）/ `shiftRemove(key)`（O(n)，严格保序）

## 许可证

MIT
