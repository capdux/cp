# @cap/smallvec

零堆分配的 CP 原生内联栈向量库，Clean-Room MIT 实现对标 crates.io Top 30 顶流包 `smallvec`（17 亿+ 下载）。

## 特性

- `SmallVecU64`（64 元素内联栈缓冲）与 `SmallVecU8`（256 字节内联栈缓冲）
- 支持 `push`、`pop`、`insert`、`remove`、`swapRemove`（O(1)）、`extendFromSlice`、`dedup`、`reverse`、`truncate`、`contains`

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`servo/rust-smallvec`](https://github.com/servo/rust-smallvec)。
