# `cap:padding_checker` — 编译期结构体零未初始化填充校验器

> 参考来源：/data/dev/bun/src/install/padding_checker.cp | 吸收日期：2026-11-22

## 概述

`cap:padding_checker` 吸收自 Bun 锁文件序列化安全守卫（`src/install/padding_checker.cp`），用于在通过 `std.mem.asBytes` / `sliceAsBytes` 将结构体直接序列化为二进制锁文件、共享内存或持久化快照前，于**编译期**递归校验结构体是否存在隐式未初始化填充字节（padding）或指针字段，彻底杜绝未初始化内存垃圾字节泄漏与跨进程失效指针写入。

## 上游背景与安全动因

- **真实隐患来源（Bun #4319）**：当结构体包含不同对齐要求的字段（例如 `a: u8` 后紧跟 `b: u64`）时，编译器会在 `a` 和 `b` 之间插入 7 字节对齐间隙，或在结构体末尾插入尾部对齐间隙。这部分隐式填充字节属于**未初始化内存**；若直接对结构体取字节切片写盘，会导致确定性哈希失效并向文件泄漏栈/堆残留内存。
- **标准修复模式**：在产生间隙的字段之间或结构体末尾添加显式零初始化填充字段（例如 `_pad: [7]u8 = [0; 7]`），或合理重排字段从大对齐到小对齐排列。
- **指针禁止规则**：包含任意指针字段的结构体无法跨进程/跨重启安全反序列化，`hasPointers` 与 `assertNoUninitializedPadding` 会在编译期递归拦截。

## 核心导出 API

| 符号 | 说明 |
| :--- | :--- |
| `paddingBytes(comptime T): usize` | 返回结构体 `T` 中的隐式填充总字节数（零运行期开销） |
| `hasPadding(comptime T): bool` | 递归检查 `T` 及嵌套结构体/数组/可选字段是否存在隐式填充 |
| `hasPointers(comptime T): bool` | 递归检查 `T` 及嵌套字段是否包含指针类型 |
| `isSafeBinarySerializable(comptime T): bool` | 等价于 `!hasPointers(T) && !hasPadding(T)` |
| `assertNoUninitializedPadding(comptime T)` | 编译期强断言，若存在指针或字段间隙/尾部填充则直接触发 `@compileError` |

## 边界注意事项

1. **不等长 `union` 限制**：与 Bun 上游一致，`assertNoUninitializedPadding` 递归检查 `union` 内部各变体字段本身，但**不会检查 `union` 中较短变体未覆盖到的共享尾部字节**；序列化带 `union` 的结构前应先清零整块内存。
2. **显式 `undefined` 赋值**：编译期布局检查只能消除隐式填充间隙，无法拦截运行期对正常字段赋 `undefined` 的行为。
3. **跨平台字节序**：通过本模块校验仅保证无隐式填充与无指针；跨架构持久化仍需关注端序（Endianness）与 `extern struct` 布局约定。

## 使用示例

```cp
import { isSafeBinarySerializable, assertNoUninitializedPadding } from "cap:padding_checker";

extern struct LockHeader {
    magic: u32,
    version: u16,
    _pad: [2]u8 = [0; 2],
    timestamp: u64,
}

comptime {
    assertNoUninitializedPadding(LockHeader);
}

const ok = isSafeBinarySerializable(LockHeader); // true
* = ok;
```
