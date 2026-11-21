# `cap:cow_str` — 零拷贝借用 / 原子引用计数写时复制字符串 `CowStr`

源自 Bun `shell/EnvStr.cp`，统一表示空串、零拷贝借用切片与原子引用计数堆字符串，支持 O(1) `clone()`、均摊内存开销 `memoryCost()` 与 `~init()` 确定性 RAII 零泄漏释放。

## 导出符号

- `CowStrKind`（`empty` / `borrowed` / `refcounted`）
- `CowStr`（含 `~init()` RAII）: `initEmpty` / `initBorrowed` / `dupeOwned` / `clone` / `slice` / `len` / `isEmpty` / `isOwned` / `isBorrowed` / `refCount` / `memoryCost` / `eql` / `eqlStr`
