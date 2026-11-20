# CP 标准库：Roaring Bitmap 压缩 32 位整数集合引擎手册 (`cap:roaring`)

## 一、 架构总览与设计准则

`cap:roaring` 是 CP 语言官方内置的纯 CP **Roaring Bitmap（压缩 32 位无符号整数集合）** 引擎，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。按高 16 位 `key` 划分为有序容器块，每个块依据基数在 `ArrayContainer`（`<= 4096` 元素有序 `u16` 数组）与 `BitsetContainer`（`> 4096` 元素固定 `1024 x u64` 8KB 位图）之间自适应切换。既可通过 `import { ... } from "cap:roaring"` 导入，也支持 `import cap from "cap"; cap.roaring.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/roaring/roaring.cp` | `// Source: cp/lib/roaring/roaring.cp (...)` | `RoaringError`、`RoaringBitmap`、自适应容器与二进制编解码 |
| `lib/roaring/index.cp` | `// Source: cp/lib/roaring/index.cp (...)` | `cap:roaring` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 `RoaringError`

- `InvalidSerializedData`：反序列化输入魔数非 `"ROAR"`、版本不匹配、容器乱序/越界、长度截断或校验不一致。
- `InvalidRange`：`addRange(start, end_excl)` 传入 `start > end_excl`。
- `OutOfMemory`：底层容器数组或输出切片内存分配失败。

### 2. `RoaringBitmap`

- **生命周期与克隆**：
  - `RoaringBitmap.init(alloc): RoaringBitmap`
  - `deinit(*_)`
  - `clone(alloc): RoaringBitmap ! RoaringError`
- **增删查与区间操作**：
  - `add(*_, val: u32): bool ! RoaringError`（新增返回 `true`，已存在返回 `false`；单块超 `4096` 自动升级为 `BitsetContainer`）
  - `addRange(*_, start: u32, end_excl: u32): void ! RoaringError`（半开区间 `[start, end_excl)`，支持跨块高速批量置位）
  - `removeVal(*_, val: u32): bool`（删除后基数 `<= 4096` 自动降级回 `ArrayContainer`，空块自动回收）
  - `contains(val: u32): bool`
- **统计、极值与排名**：
  - `cardinality(): u64` / `isEmpty(): bool`
  - `minimum(): u32?` / `maximum(): u32?`
  - `rank(val: u32): u64`（返回集合中 `<= val` 的元素总数）
- **集合代数运算**：
  - `bitAnd(alloc, other): RoaringBitmap ! RoaringError`（交集）
  - `bitOr(alloc, other): RoaringBitmap ! RoaringError`（并集）
  - `bitAndNot(alloc, other): RoaringBitmap ! RoaringError`（差集 `self \ other`）
  - `bitXor(alloc, other): RoaringBitmap ! RoaringError`（对称差）
  - `andCardinality(other): u64` / `intersects(other): bool`（零堆分配快速计算）
- **导出与二进制序列化**：
  - `toSlice(alloc): []u32 ! RoaringError`（严格升序导出全部元素，调用方通过 `alloc.free` 释放）
  - `serialize(alloc): []u8 ! RoaringError`（带 `"ROAR"` 魔数头与容器元信息，调用方通过 `alloc.free` 释放）
  - `RoaringBitmap.deserialize(alloc, bytes: []const u8): RoaringBitmap ! RoaringError`

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { RoaringBitmap } from "cap:roaring";
import cap from "cap";

pub fn demoRoaring(): ! {
    const alloc = std.heap.page_allocator;

    let a = RoaringBitmap.init(alloc);
    defer a.deinit();
    * = a.add(42)?;
    a.addRange(65500, 70000)?;

    let b = cap.roaring.RoaringBitmap.init(alloc);
    defer b.deinit();
    b.addRange(69000, 71000)?;

    let inter = a.bitAnd(alloc, b)?;
    defer inter.deinit();
    const count = inter.cardinality();
    * = count;

    const raw = inter.serialize(alloc)?;
    defer alloc.free(raw);
    let restored = RoaringBitmap.deserialize(alloc, raw)?;
    defer restored.deinit();
}
```
