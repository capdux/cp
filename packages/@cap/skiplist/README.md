# @cap/skiplist

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:skiplist` ([https://github.com/JP-Ellis/rust-skiplist](https://github.com/JP-Ellis/rust-skiplist))

Zero-allocation static-pool deterministic skip list (`MAX_NODES = 128`, `MAX_LEVEL = 8`) for CP — clean-room MIT parity with Rust `skiplist` and `crossbeam-skiplist`.

零堆分配的 CP 确定性静态节点池跳表，内置 xorshift64 确定性层高与 O(1) 空闲节点池复用，提供 O(log N) 有序插入、查询、删除、`first`/`last`、`lowerBound` 与 `rangeCount` 闭区间统计，适用于内存数据库 MemTable、排行榜与有序索引（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/skiplist
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `SkipEntry` | `pub struct { key: i64, value: i64, found: bool }` | 查询结果结构体 |
| `SkipList.init` | `() -> SkipList` | 初始化空跳表（128 个可复用节点槽位） |
| `insert` / `remove` / `clear` | `(*_, ...)` | O(log N) 插入/覆盖更新、删除回收与重置 |
| `get` / `contains` | `(key: i64) -> SkipEntry / bool` | 精确查找与成员检测 |
| `first` / `last` / `lowerBound` | `() / (key: i64) -> SkipEntry` | 最小键、最大键与首个 `>= key` 定位 |
| `rangeCount` | `(min_key: i64, max_key: i64) -> usize` | 统计闭区间 `[min_key, max_key]` 条目数 |
| `len` / `isEmpty` | 状态查询 | 长度与判空 |

## License

MIT
