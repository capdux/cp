# CP 标准库：HyperLogLog 基数估算与分布式可合并草图手册 (`cap:hll`)

## 一、 架构总览与设计准则

`cap:hll` 是 CP 语言官方内置的纯 CP **HyperLogLog 基数估算与分布式可合并草图库**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。采用强混合 64-bit SplitMix64 / FNV-1a 哈希管线、LinearCounting 小基数无偏修正、64-bit 大基数修正、无损逐桶 `max` 分布式聚合与紧凑版本化二进制序列化（`"HLL1"`）。支持 `import { HyperLogLog, HllError } from "cap:hll"` 与 `import cap from "cap"; cap.hll.*` 动态挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/hll/hll.cp` | `// Source: cp/lib/hll/hll.cp (...)` | `HllError`、`HyperLogLog` 核心算法、分片合并与 `"HLL1"` 二进制编解码 |
| `lib/hll/index.cp` | `// Source: cp/lib/hll/index.cp (...)` | `cap:hll` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 (`HllError`)

- **`InvalidPrecision`**：精度参数 `precision` 超出合法闭区间 `[4, 16]`。
- **`PrecisionMismatch`**：`merge` 时两个 `HyperLogLog` 实例的精度 `p`（或寄存器数量 `m`）不一致。
- **`InvalidSerializedData`**：`deserialize` 时魔数非 `"HLL1"`、头长度不足 5 字节、头内精度越界或载荷长度不等于 `5 + 2^p`。
- **`OutOfMemory`**：寄存器缓冲区或序列化输出缓冲区内存分配失败。

### 2. `HyperLogLog` 结构体

- **`init(allocator: std.mem.Allocator, precision: u8): HyperLogLog ! HllError`**：校验 `precision` 在 `[4, 16]` 内，分配 `m = 1 << precision` 个 `u8` 寄存器并清零。
- **`deinit(*_)`**：释放底层寄存器切片内存。
- **`clear(*_)`**：将所有寄存器清零而不重新分配内存。
- **`precision(): u8`**：返回配置精度 `p`。
- **`registerCount(): usize`**：返回寄存器桶数量 `m = 2^p`。
- **`relativeErrorBound(): f64`**：返回理论标准相对误差上界 `1.04 / sqrt(m)`。
- **`add(*_, item: []const u8)`**：对字节切片/字符串执行 FNV-1a + SplitMix64 混合后写入寄存器。
- **`addHash(*_, hash64: u64)`**：对 64-bit 整型/哈希值执行 SplitMix64 强混合；高 `p` 位选择桶索引，低 `64 - p` 位计算前导零计数 `+ 1` 并更新对应寄存器最大值。
- **`count(): u64`**：调和平均估算 + 小基数（`E <= 2.5m` 且存在零桶）LinearCounting 修正 + 大基数 64-bit 修正。
- **`merge(*_, other: HyperLogLog): void ! HllError`**：要求 `precision` 一致（否则返回 `HllError.PrecisionMismatch`），逐桶取 `max` 实现无损分布式聚合。
- **`serialize(allocator: std.mem.Allocator): []u8 ! HllError`**：编码为 `"HLL1"` (4B) + `precision` (1B) + `registers` (`m` B) 的二进制切片。
- **`deserialize(allocator: std.mem.Allocator, bytes: []const u8): HyperLogLog ! HllError`**：校验 `"HLL1"` 魔数、精度头与总长度并还原实例。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { HyperLogLog, HllError } from "cap:hll";

pub fn demoHll(): ! {
    const alloc = std.testing.allocator;
    let hll = HyperLogLog.init(alloc, 14)?;
    defer hll.deinit();

    hll.add("user:1001");
    hll.add("user:1002");
    hll.add("user:1001");
    hll.addHash(42);

    const est = hll.count();
    * = est;

    const bytes = hll.serialize(alloc)?;
    defer alloc.free(bytes);

    let restored = HyperLogLog.deserialize(alloc, bytes)?;
    defer restored.deinit();
}
```
