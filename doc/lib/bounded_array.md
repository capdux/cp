# `cap:bounded_array` — 栈上定容动态数组 `BoundedArray<T, capacity>`

> 参考来源：/data/dev/bun/src/collections/bounded_array.cp | 吸收日期：2026-11-22

## 概述

`cap:bounded_array` 吸收自 Bun 集合库（`src/collections/bounded_array.cp`，沿袭并优化自 Zig 标准库在 PR #24699 移除前的 `BoundedArray`），提供 100% 零堆分配、编译期固定容量的栈上动态数组 `BoundedArray(T, buffer_capacity)`，适合编译期已知最大上限、运行期实际长度可变的协议帧、AST 小列表与路径分段容器，完全无需 `Allocator`。

## 上游设计背景与行为契约

- **无分配器值语义**：数组底层直接内联 `[buffer_capacity]T` 与长度字段，整体可按值传递或放置在栈上/结构体中，离开作用域无需释放。
- **确定性溢出与越界防护**：所有写入或扩容操作（`append`、`appendSlice`、`appendNTimes`、`insert`、`fromSlice`、`resize`）在超出 `buffer_capacity` 时均返回 `BoundedArrayError.Overflow`，索引非法时返回 `BoundedArrayError.OutOfBounds`，绝不静默截断或越界写入。
- **双删除语义**：
  - `orderedRemove(index)`：保持剩余元素相对顺序，复杂度 $O(n)$；
  - `swapRemove(index)`：用末尾元素填补空位，复杂度 $O(1)$，不保留顺序。

## 导出符号与 API

### 错误集

- `BoundedArrayError`：`Overflow` / `OutOfBounds`

### `BoundedArray(comptime T: type, comptime buffer_capacity: usize)`

| 方法 | 说明 |
| :--- | :--- |
| `init(len)` / `fromSlice(src)` | 初始化指定长度或从已有切片拷贝，超容返回 `Overflow` |
| `slice()` / `constSlice()` | 返回当前有效元素范围的切片视图 |
| `capacity()` / `len` | 查询固定容量与当前有效长度 |
| `append(item)` / `appendSlice(items)` / `appendNTimes(item, n)` | 尾部追加单个/多个/重复元素，超容返回 `Overflow` |
| `insert(index, item)` | 在指定位置插入元素，越界返回 `OutOfBounds`，满容返回 `Overflow` |
| `pop()` | 弹出并返回尾部元素，空数组返回 `null` |
| `orderedRemove(index)` / `swapRemove(index)` | 按序删除或 $O(1)$ 交换删除 |
| `resize(new_len)` / `clear()` | 调整有效长度或清空重置 `len = 0` |

## 边界注意事项

1. **`resize` 扩容未初始化语义**：与上游一致，将 `len` 调大时新增槽位不自动清零，调用方需负责填充后再读取。
2. **值拷贝注意切片失效**：由于底层缓冲内联在结构体中，对 `BoundedArray` 进行值拷贝后，原实例的 `slice()` 仍指向旧实例的栈内存，不应跨拷贝保留旧切片。

## 使用示例

```cp
import { BoundedArray, BoundedArrayError } from "cap:bounded_array";

const SmallInts = BoundedArray(i32, 8);
let arr = SmallInts.init(0)?;
arr.append(10)?;
arr.appendSlice([20, 30][0..])?;
const last = arr.pop(); // 30
* = last;
* = BoundedArrayError.Overflow;
```
