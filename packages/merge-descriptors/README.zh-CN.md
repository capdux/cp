[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# merge-descriptors

Zero-allocation property descriptor table and safe descriptor merge engine for CP (clean-room parity with npm merge-descriptors v2)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:merge-descriptors` ([https://github.com/component/merge-descriptors](https://github.com/component/merge-descriptors))

| 元信息 | 值 |
| --- | --- |
| 包名 | `merge-descriptors` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add merge-descriptors
```

```cp
import {
    PropDescriptor,
    DescriptorTable,
    mgdStrEq,
    mgdEmptyDesc,
    mergeDescriptors,
    countEnumerable,
} from "merge-descriptors";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PropDescriptor` | `class` | 字段: `key: string, value: string, writable: bool, enumerable: bool, configurable: bool` · 方法: `data, readonly` |
| `DescriptorTable` | `class` | 字段: `entries: [32]PropDescriptor, count: usize` · 方法: `new, len, has, get, define, deleteKey` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mgdStrEq` | `(a: string, b: string): bool` | 调用 `mgdStrEq` 并返回 `bool`。 |
| `mgdEmptyDesc` | `(): PropDescriptor` | 调用 `mgdEmptyDesc` 并返回 `PropDescriptor`。 |
| `mergeDescriptors` | `(dst: &mut DescriptorTable, src: DescriptorTable, redefine: bool): usize` | 调用 `mergeDescriptors` 并返回 `usize`。 |
| `countEnumerable` | `(tbl: DescriptorTable): usize` | 调用 `countEnumerable` 并返回 `usize`。 |

## 最小调用示例

```cp
import { mgdStrEq } from "merge-descriptors";

pub fn main() {
    let result = mgdStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
