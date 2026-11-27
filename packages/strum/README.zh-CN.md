[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# strum

Zero-heap enum variant string registry, case-insensitive parser, discriminant lookup, and iteration (clean-room port of Rust strum)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:strum` ([https://github.com/Peternator7/strum](https://github.com/Peternator7/strum))（原库名 `strum` → CP 包名 `strum`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `strum` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add strum
```

```cp
import { EnumVariant, EnumRegistry, stmStrEq } from "strum";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `EnumVariant` | `class` | 字段: `id: i32, name: string, alias: string, message: string` |
| `EnumRegistry` | `class` | 字段: `variants: [32]EnumVariant, count: usize` · 方法: `new, variantCount, register, fromStr, fromStrIgnoreCase, asStr, +3 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `stmStrEq` | `(a: string, b: string): bool` | 调用 `stmStrEq` 并返回 `bool`。 |

## 最小调用示例

```cp
import { stmStrEq } from "strum";

pub fn main() {
    let result = stmStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
