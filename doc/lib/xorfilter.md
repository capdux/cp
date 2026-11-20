# CP 标准库：极简空间静态 Xor 过滤器手册 (`cap:xorfilter`)

## 一、 架构总览与设计准则

`cap:xorfilter` 是 CP 语言官方内置的纯 CP **静态 Xor 过滤器（Xor8 / Xor16 Static Minimal-Space Approximate Set Membership Filter）**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，基于 3-peeling 超图剥离构建与逆序 XOR 指纹赋值算法实现。相比传统 Bloom Filter 与 Cuckoo Filter，在已知静态集合（如词典、路由表、黑名单快照、SSTable 索引）场景下具备**极致的每键比特数（Xor8 ≈ 9.84 bits/key，Xor16 ≈ 19.68 bits/key）**与**恒定 3 次数组访存 $O(1)$ 查询**，同时保证**零假阴性（Zero False Negatives）**。

既可通过 `import { ... } from "cap:xorfilter"` 导入，也支持 `import cap from "cap"; cap.xorfilter.*` 动态懒挂载调用。

### 核心算法参数与不变量
- **3-段超图映射（3-Peeling）**：底层指纹数组划分为 3 个等长段（总长度 `3 * block_length`，容量约为 `1.23 * N + 32` 向上取整至 3 的倍数）。每个 64-bit 键通过独立混合与 FastRange 映射至 3 个互不相交段中的槽位 `(h0, h1, h2)`。
- **自动去重与 Seed 重试**：`build` / `buildHashes` 会自动排序并去重输入键/哈希；若当前 `seed` 存在 2-core 环导致剥离未覆盖全部唯一键，自动切换 `seed` 重试（最多 64 次），若仍失败返回 `XorFilterError.ConstructionFailed`。
- **空集合边界**：传入空切片时安全返回空过滤器（`len() == 0`, `byteSize() == 0`, `bitsPerKey() == 0.0`），所有 `contains` / `containsHash` 恒定返回 `false`，且支持序列化往返。
- **二进制自描述格式**：`serialize` / `deserialize` 携带 32 字节小端序头部（4 字节魔数 `"XOR8"` / `"XOR6"` + 4 字节版本号 `1` + `seed` + `block_length` + `item_count`），严格校验长度与边界，拦截截断或损坏报文并返回 `XorFilterError.InvalidSerializedData`。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/xorfilter/xorfilter.cp` | `// Source: cp/lib/xorfilter/xorfilter.cp (...)` | `XorFilterError`、`Xor8Filter`、`Xor16Filter` |
| `lib/xorfilter/index.cp` | `// Source: cp/lib/xorfilter/index.cp (...)` | `cap:xorfilter` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 `XorFilterError`
- `ConstructionFailed`：超过最大 `seed` 重试次数仍无法完成 3-peeling 剥离，或集合规模溢出安全上限。
- `InvalidSerializedData`：反序列化输入长度不足、魔数不匹配、版本号不支持、头部与负载长度不一致或存在溢出。
- `OutOfMemory`：临时构建缓冲区或底层指纹切片内存分配失败。

### 2. `Xor8Filter`（8-bit 指纹，假阳性率 ~0.39%，约 9.84 bits/key）
- **构建与释放**：
  - `Xor8Filter.build(allocator: std.mem.Allocator, keys: []const []const u8): Xor8Filter ! XorFilterError`
  - `Xor8Filter.buildHashes(allocator: std.mem.Allocator, hashes: []const u64): Xor8Filter ! XorFilterError`
  - `deinit(*_)`
- **查询（O(1) 恒定 3 次数组访存）**：
  - `contains(key: []const u8): bool`
  - `containsHash(h: u64): bool`
- **序列化与状态**：
  - `serialize(allocator: std.mem.Allocator): []u8 ! XorFilterError`
  - `Xor8Filter.deserialize(allocator: std.mem.Allocator, data: []const u8): Xor8Filter ! XorFilterError`
  - `len(): usize` / `byteSize(): usize` / `bitsPerKey(): f64`

### 3. `Xor16Filter`（16-bit 指纹，假阳性率 ~0.0015%，约 19.68 bits/key）
- 提供与 `Xor8Filter` 完全对称的静态构建、查询、二进制序列化与状态查询接口：
  - `Xor16Filter.build` / `Xor16Filter.buildHashes` / `deinit(*_)`
  - `contains` / `containsHash`
  - `serialize` / `Xor16Filter.deserialize`
  - `len` / `byteSize` / `bitsPerKey`

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { Xor8Filter, Xor16Filter, XorFilterError } from "cap:xorfilter";
import cap from "cap";

pub fn main() {
    const alloc = std.heap.page_allocator;

    const keys: [3]string = [ "alpha", "beta", "gamma" ];
    let f8 = Xor8Filter.build(alloc, &keys) catch return;
    defer f8.deinit();

    const hit = f8.contains("alpha"); // true
    const blob = f8.serialize(alloc) catch return;
    defer alloc.free(blob);

    let restored = Xor8Filter.deserialize(alloc, blob) catch return;
    defer restored.deinit();

    // 通过 cap.xorfilter 动态挂载调用
    let f16 = cap.xorfilter.Xor16Filter.build(alloc, &keys) catch return;
    defer f16.deinit();
    _ = hit;
}
```
