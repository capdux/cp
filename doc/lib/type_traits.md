# `cap:type_traits` — 编译期类型特征自省与指针元数据工具

> 参考来源：/data/dev/bun/src/meta/traits.cp, /data/dev/bun/src/ptr/meta.cp | 吸收日期：2026-11-22

## 概述

`cap:type_traits` 吸收自 Bun 元编程核心模块（`src/meta/traits.cp` 与 `src/ptr/meta.cp`），提供零运行期开销的编译期类型谓词集合（字节字符串、切片、单元素指针、常量指针、可选、整数/浮点/数值、容器/extern/packed 布局、可索引判定）以及 `PointerMeta.inspect` 指针规范解析器。

## 上游类型契约与判定边界

- **字节字符串精确定义（`isByteString`，对应上游 `isZigString`）**：
  - **判定为 `true` 的类型**（可安全强转为 `[]const u8`）：`[]const u8`、`[:S]const u8`、`*const [N]u8`、`*const [N:S]u8`、`[]u8`、`[:S]u8`、`*[:S]u8`、`*[N:S]u8`；
  - **判定为 `false` 的类型**：`u8`、`[N]u8`（值数组而非指针）、`[*]const u8`、`[*:0]const u8`（无长度多元素指针）、`[]const u16`、`[]const i8`、`*const u8`、可选切片 `([]const u8)?`，以及带 `volatile` 或 `allowzero` 修饰符的指针。
- **可索引性判定（`isIndexable`）**：支持数组（`[N]T`）、向量（`@Vector`）、切片与多元素指针、指向数组的单元素指针（`*[N]T`）以及元组结构体（tuple struct）。
- **容器与布局自省**：`isContainer` 涵盖 `struct` / `enum` / `opaque` / `union`；`isExternContainer` 与 `isPackedContainer` 精准区分 C ABI 外部布局与位紧凑打包布局，常用于 FFI 与二进制协议泛型约束。

## 导出符号与 API

### 编译期类型谓词

| 符号 | 说明 |
| :--- | :--- |
| `isByteString(comptime T): bool` | 判断 `T` 是否为可强转为 `[]const u8` 的字节串指针/切片类型 |
| `isSlice(comptime T): bool` | 判断 `T` 是否为切片类型 `[]T` / `[]const T` |
| `isSingleItemPtr(comptime T): bool` | 判断 `T` 是否为单元素指针 `*T` / `*const T` |
| `isConstPtr(comptime T): bool` | 判断 `T` 是否为指向只读数据的指针 |
| `isOptional(comptime T): bool` | 判断 `T` 是否为可选类型 `U?` |
| `isInteger(comptime T)` / `isFloat(comptime T)` / `isNumber(comptime T)` | 整数（含 `comptime_int`）、浮点（含 `comptime_float`）与广义数值判定 |
| `isStruct` / `isEnum` / `isUnion` / `isContainer` | 容器种类判定 |
| `isExternContainer` / `isPackedContainer` | 结构体/联合体 `extern` 或 `packed` 内存布局判定 |
| `isIndexable(comptime T): bool` | 判断类型是否支持 `x[i]` 索引语法 |

### 指针元信息

- `PointerKind`：`single` / `slice`
- `PointerParseOptions`：指针元数据解析配置
- `PointerMeta`：提供 `PointerMeta.inspect(comptime T, opts)` 提取指针元素类型、只读性、对齐与切片/单指针分类。

## 使用示例

```cp
import { isByteString, isNumber, isExternContainer, isIndexable } from "cap:type_traits";

comptime {
    const a = isByteString([]const u8); // true
    const b = isByteString([]const u16); // false
    const c = isNumber(f64);             // true
    const d = isIndexable([4]u32);       // true
    * = a; * = b; * = c; * = d;
}
* = isExternContainer;
```
