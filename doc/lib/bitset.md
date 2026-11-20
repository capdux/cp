# CP 标准库：64-bit 字级动态位图与紧凑固定位图手册 (`cap:bitset`)

## 一、 架构总览与设计准则

`cap:bitset` 是 CP 语言官方内置的纯 CP **64-bit 字级动态位图（`BitSet`）与零堆分配栈上固定位图（`FixedBitSet(N)`）** 引擎，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为倒排索引交并加速、图算法 `Visited` 集合、RBAC 位掩码权限与大规模整数去重打造。既可通过 `import { ... } from "cap:bitset"` 导入，也支持 `import cap from "cap"; cap.bitset.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/bitset/bitset.cp` | `// Source: cp/lib/bitset/bitset.cp (...)` | `BitSetError`、`BitSet`、`FixedBitSet(N)` |
| `lib/bitset/index.cp` | `// Source: cp/lib/bitset/index.cp (...)` | `cap:bitset` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 `BitSetError`

- `InvalidRange`：区间操作传入 `start_bit > end_bit`。
- `InvalidHex`：`fromHex` 传入奇数长度或包含非十六进制字符的输入。
- `OutOfMemory`：底层 64-bit 字数组或输出切片分配失败。

### 2. `BitSet`（动态扩容 `u64` 字数组位图）

- **生命周期与克隆**：
  - `BitSet.init(alloc): BitSet`
  - `BitSet.initWithCapacity(alloc, bit_capacity): BitSet ! BitSetError`
  - `deinit(*_)`
  - `clone(alloc): BitSet ! BitSetError`
- **单点与区间操作**：
  - `set(*_, bit): void ! BitSetError` / `clear(*_, bit)` / `toggle(*_, bit): void ! BitSetError` / `testBit(bit): bool`
  - `setRange(*_, start_bit, end_bit): void ! BitSetError` / `clearRange(*_, start_bit, end_bit): void ! BitSetError`（半开区间 `[start_bit, end_bit)`，自动跨 64-bit 字边界）
  - `clearAll(*_)`
- **硬件加速统计与遍历**：
  - `count(): usize`（基于 `@popCount` 累加）
  - `isEmpty(): bool`
  - `firstSetBit(): usize?` / `nextSetBit(from_bit): usize?`（基于 `@ctz` 跳零扫描）
  - `toIndices(alloc): []usize ! BitSetError`（基于 `@ctz` 与 `w &= w - 1` 收集全部置位下标，调用方使用 `alloc.free` 释放）
- **集合代数（自动对齐不等长位图）**：
  - `unionWith(*_, other): void ! BitSetError`（`self |= other`）
  - `intersectWith(*_, other)`（`self &= other`，超出 `other` 长度的高位自动清零）
  - `differenceWith(*_, other)`（`self &= ~other`）
  - `xorWith(*_, other): void ! BitSetError`（`self ^= other`）
  - `intersects(other): bool` / `isSubsetOf(other): bool` / `equals(other): bool`
  - `jaccardSimilarity(other): f64`（`|A ∩ B| / |A ∪ B|`；若两集合均为空返回 `1.0`）
- **紧凑十六进制序列化**：
  - `toHex(alloc): []u8 ! BitSetError`（小端字节序、自动裁去尾部全零字节，调用方用 `alloc.free` 释放）
  - `BitSet.fromHex(alloc, hex_str): BitSet ! BitSetError`

### 3. `FixedBitSet(comptime N: usize)`（零堆栈上固定 `N` 位位图）

- 编译期计算字数 `WORD_COUNT = (N + 63) / 64`，纯栈上 `[WORD_COUNT]u64` 布局，零堆分配。
- **`init(): Self`** / **`set(*_, bit)`** / **`clear(*_, bit)`** / **`toggle(*_, bit)`** / **`testBit(bit): bool`** / **`clearAll(*_)`**
- **`count(): usize`** / **`isEmpty(): bool`** / **`unionWith(*_, other)`** / **`intersectWith(*_, other)`** / **`equals(other): bool`**

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { BitSet, FixedBitSet } from "cap:bitset";
import cap from "cap";

pub fn demoBitSet(): ! {
    const alloc = std.heap.page_allocator;

    let a = BitSet.init(alloc);
    defer a.deinit();
    a.set(1)?;
    a.setRange(60, 130)?;

    let b = cap.bitset.BitSet.init(alloc);
    defer b.deinit();
    b.setRange(64, 132)?;

    const sim = a.jaccardSimilarity(b);
    * = sim;

    const hex = a.toHex(alloc)?;
    defer alloc.free(hex);
    let restored = BitSet.fromHex(alloc, hex)?;
    defer restored.deinit();

    let fixed = FixedBitSet(256).init();
    fixed.set(0);
    fixed.set(255);
}
```
