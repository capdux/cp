# @cap/trie

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:radix_trie` ([https://github.com/michaelsproul/rust_radix_trie](https://github.com/michaelsproul/rust_radix_trie))（原库名 `radix_trie` → CP 包名 `@cap/trie`）

Zero-allocation LCRS (Left-Child Right-Sibling) static-pool byte prefix tree (`Trie`) for CP — clean-room MIT parity with Rust `radix_trie` / `qp-trie` and npm `trie-search`.

零堆分配的 CP 静态内存池字节前缀树（`MAX_TRIE_NODES = 512`），采用紧凑左孩子-右兄弟（LCRS）节点表示支持任意 `0..255` 字节键，提供精确插入/查询/删除、前缀存在性检测、前缀词条计数与最长前缀匹配（LPM），适用于 CLI 自动补全、HTTP/RPC 路由表与词典过滤（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/trie
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `TrieNode` / `LongestMatch` | `pub struct` | LCRS 节点与最长前缀匹配结果结构体 |
| `Trie.init` | `() -> Trie` | 初始化空前缀树（节点 0 为根） |
| `insert` / `remove` / `clear` | `(*_, ...)` | 原子插入/覆盖更新、逻辑删除（同步维护 `subtree_words`）与重置 |
| `get` / `contains` | `(key: []const u8) -> i64 / bool` | 精确查值（不存在返回 `-1`）与存在性判断 |
| `hasPrefix` / `countWithPrefix` | `(prefix: []const u8) -> bool / usize` | 前缀存在性检测与前缀下存活键计数 |
| `longestPrefixMatch` | `(text: []const u8) -> LongestMatch` | 最长前缀匹配（LPM） |
| `len` / `nodeCount` | `() -> usize` | 存活键总数与静态池已分配节点数 |

## License

MIT
