[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# relative-path

Portable platform-independent relative path manipulation, normalization, joining, and resolution (zero-heap pure CP)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`relative-path`](https://github.com/udoprog/relative-path)（`MIT OR Apache-2.0`），以 Clean-room MIT 纯 CP 零堆方式实现，包名 `relative-path`。

| 元信息 | 值 |
| --- | --- |
| 包名 | `relative-path` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add relative-path
```

```cp
import {
    RelPath,
    rpIsSep,
    normalizeRelPath,
    joinRelPath,
    relativeTo,
} from "relative-path";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RelPath` | `class` | 字段: `buf: [256]u8, length: usize` · 方法: `empty, fromStr, len, isEmpty, equals, copyTo, +10 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rpIsSep` | `(b: u8): bool` | 调用 `rpIsSep` 并返回 `bool`。 |
| `normalizeRelPath` | `(path: string): RelPath` | 调用 `normalizeRelPath` 并返回 `RelPath`。 |
| `joinRelPath` | `(base: string, rel: string): RelPath` | 调用 `joinRelPath` 并返回 `RelPath`。 |
| `relativeTo` | `(from_path: string, to_path: string): RelPath` | 调用 `relativeTo` 并返回 `RelPath`。 |

## 最小调用示例

```cp
import { normalizeRelPath } from "relative-path";

pub fn main() {
    let result = normalizeRelPath("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
