# `cap:cow_str` — 零拷贝借用 / 原子引用计数写时复制字符串 `CowStr`

> 参考来源：/data/dev/bun/src/shell/EnvStr.cp | 吸收日期：2026-11-22

源自 Bun `shell/EnvStr.cp`，统一表示空串、零拷贝借用切片与原子引用计数堆字符串，支持 O(1) `clone()`、均摊内存开销 `memoryCost()` 与 `~init()` 确定性 RAII 零泄漏释放。

## 导出符号

- `CowStrKind`（`empty` / `borrowed` / `refcounted`）
- `CowStr`（含 `~init()` RAII）: `initEmpty` / `initBorrowed` / `dupeOwned` / `clone` / `slice` / `len` / `isEmpty` / `isOwned` / `isBorrowed` / `refCount` / `memoryCost` / `eql` / `eqlStr`

## 上游行为契约与共享所有权边界

1. **三态统一存储与 O(1) 克隆（源自 Bun Shell `EnvStr`）**：
   - `empty` 与 `borrowed` 形态完全零堆分配，`clone()` 为纯值拷贝；`refcounted` 形态通过原子引用计数共享同一堆缓冲，`clone()` 仅执行一次原子自增（O(1) 无字节拷贝），适合跨任务/子进程环境变量快照共享。
2. **借用生命周期约束**：
   - `initBorrowed(slice)` 不拷贝底层字节，调用方须确保源切片生命周期覆盖该 `CowStr` 及其所有 `clone()`；若需脱离源生命周期独立持有，须通过 `dupeOwned(alloc, slice)` 转为 `refcounted`。
3. **RAII 确定性析构**：
   - 最后一个引用离开作用域时 `~init()` 自动释放底层堆块，`memoryCost()` 返回按 `refCount()` 均摊后的估算字节数便于内存配额核算。
