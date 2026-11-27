# @cap/sieve-cache

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:sieve-cache` ([https://github.com/jedisct1/rust-sieve-cache](https://github.com/jedisct1/rust-sieve-cache))

Zero-allocation NSDI'24 **SIEVE** cache eviction engine for CP — clean-room MIT parity with Rust `sieve-cache` and `quick_cache`.

零堆分配的 CP NSDI'24 SIEVE 缓存淘汰引擎，采用静态双向链表池（最大容量 128）与开放寻址哈希索引，命中时不移动链表节点、仅置 `visited = true`，淘汰时由 `hand` 指针快速扫描驱逐，兼具极简结构、高命中率与抗扫描污染能力（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/sieve-cache
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `SieveCache.init` | `(capacity: usize) -> SieveCache` | 初始化缓存（容量限制在 `1..=128`） |
| `put` / `get` / `remove` / `clear` | `(*_, ...)` | 插入/更新、命中标记并取值、安全删除与清空重置 |
| `peek` / `contains` | `(key: u64) -> i64 / bool` | 只读探测，不修改 `visited` 与统计 |
| `len` / `capacity` / `isEmpty` | 状态查询 | 容量与长度查询 |
| `hits` / `misses` / `evictions` | `() -> u64` | 命中、未命中与淘汰计数统计 |

## License

MIT
