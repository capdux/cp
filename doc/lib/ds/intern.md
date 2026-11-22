# CP 标准库：高性能字符串驻留池与双向唯一映射表手册 (`cap:intern`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ds`，导入请使用 `import { ... } from "cap:ds"`。

## 一、 架构总览与设计准则

`cap:intern` 是 CP 语言官方内置的纯 CP **高性能字符串驻留池（`StringInterner`）与严格 1:1 双向唯一映射表（`StringBiMap`）** 引擎，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为编译器符号表、图知识库节点映射、协议字典压缩与高频字符串去重打造。既可通过 `import { ... } from "cap:ds"` 导入，也支持 `import cap from "cap"; cap.intern.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/intern/intern.cp` | `// Source: cp/lib/intern/intern.cp (...)` | `InternError`、`SymbolId`、`StringInterner`、`StringBiMap` |
| `lib/intern/index.cp` | `// Source: cp/lib/intern/index.cp (...)` | `cap:intern` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与类型

- `pub type SymbolId = u32`：紧凑 32 位符号标识，支持 $O(1)$ 整数相等比较。
- `pub err InternError`：
  - `InvalidSymbolId`：`resolve(id)` 传入越界的 `SymbolId`。
  - `KeyAlreadyExists`：`StringBiMap.insert` 时 `key` 已绑定其他值。
  - `ValueAlreadyExists`：`StringBiMap.insert` 时 `val` 已绑定其他键。
  - `OutOfMemory`：底层 Arena 分块或哈希索引分配失败。

### 2. `StringInterner`（分块 Arena + 哈希索引字符串驻留池）

每个唯一字符串在指针稳定的 Arena 分块中仅存一份连续拷贝，返回切片在 `deinit()` 前永久有效、零拷贝还原：

- **`StringInterner.init(alloc): StringInterner`**：初始化空驻留池（零堆分配直到首次 `intern`）。
- **`deinit(*_)`**：释放全部分块、条目表与哈希索引。
- **`intern(*_, s: string): SymbolId ! InternError`**：若 `s` 已驻留则直接返回原 `SymbolId`（绝不重复分配内存）；首次出现则拷贝入 Arena 并分配递增 `SymbolId`。
- **`lookup(s: string): SymbolId?`**：只读查询 `s` 对应的 `SymbolId`，不存在返回 `null` 且不污染池。
- **`resolve(id: SymbolId): string ! InternError`**：零拷贝还原 `SymbolId` 对应的字符串，越界返回 `InternError.InvalidSymbolId`。
- **`resolveOpt(id: SymbolId): string?`**：安全还原 `SymbolId`，越界返回 `null`。
- **`count(): usize`**：返回当前已驻留的唯一字符串数量。
- **`totalBytes(): usize`**：返回去重后实际存储的字符总字节数。

### 3. `StringBiMap`（严格 1:1 双向唯一映射表）

维护 `key <-> val` 严格双射关系，内部独立持有字符串拷贝：

- **`StringBiMap.init(alloc): StringBiMap`** / **`deinit(*_)`**
- **`insert(*_, key: string, val: string): void ! InternError`**：严格双射插入；若 `key` 已绑定其他值返回 `KeyAlreadyExists`，若 `val` 已绑定其他键返回 `ValueAlreadyExists`。
- **`forcePut(*_, key: string, val: string): void ! InternError`**：原子清理 `key` 原关联与 `val` 原关联后建立新的 1:1 映射。
- **`getByKey(key: string): string?`** / **`getByVal(val: string): string?`**
- **`removeByKey(*_, key: string): bool`** / **`removeByVal(*_, val: string): bool`**
- **`len(): usize`** / **`containsKey(key: string): bool`** / **`containsVal(val: string): bool`**

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { StringInterner, StringBiMap } from "cap:ds";
import cap from "cap";

pub fn demoIntern(): ! {
    const alloc = std.heap.page_allocator;

    let pool = StringInterner.init(alloc);
    defer pool.deinit();

    const id1 = pool.intern("identifier")?;
    const id2 = pool.intern("identifier")?;
    // id1 == id2, totalBytes() == 10
    * = id1 == id2;

    const text = pool.resolve(id1)?;
    * = text;

    let bimap = cap.intern.StringBiMap.init(alloc);
    defer bimap.deinit();
    bimap.insert("node_a", "10.0.0.1")?;
    bimap.forcePut("node_b", "10.0.0.1")?; // 原 node_a <-> 10.0.0.1 自动清理
}
```
