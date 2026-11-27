# @cap/litemap

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:litemap` ([https://github.com/unicode-org/icu4x](https://github.com/unicode-org/icu4x))

零堆分配的 CP 紧凑有序二分查找映射表 — 洁净室 MIT 实现，对标 Rust ICU4X `litemap`（crates.io #168，8.24 亿+ 总下载）。平坦有序数组存储、零哈希表开销、`O(log N)` 二分查找与天然按键升序遍历。

Zero-allocation compact sorted flat-array map with `O(log N)` binary search and deterministic ordered iteration for CP — clean-room MIT parity with Rust ICU4X `litemap` (824M+ downloads).

## 安装 / Install

```bash
cap add @cap/litemap
```

## API 参考 / API Reference

- **`LiteMapStr`**（容量 64，`string -> string`）与 **`LiteMapU64`**（容量 64，`u64 -> i64`）：
  - `init()`, `len()`, `isEmpty()`, `isFull()`, `clear(*_)`
  - `binarySearch(key) -> SearchResult{ found, index }`
  - `get(key) -> string? / i64?`, `containsKey(key) -> bool`
  - `insert(*_, key, value) -> StrInsertResult / U64InsertResult{ ok, replaced, prev }`
  - `remove(*_, key) -> string? / i64?`
  - `entryAt(idx) -> StrEntry? / U64Entry?`

## 使用示例 / Usage

```cp
import { LiteMapStr, LiteMapU64 } from "@cap/litemap";

let m = LiteMapStr.init();
* = m.insert("zh", "Chinese");
* = m.insert("en", "English");
* = m.insert("de", "German");
// Ordered iteration: "de" -> "en" -> "zh"
const v = m.get("en").?; // "English"
```

## License

MIT
