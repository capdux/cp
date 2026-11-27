[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# nth-check

Zero-allocation CSS :nth-child(An+B) microsyntax parser and index matcher

| 元信息 | 值 |
| --- | --- |
| 包名 | `nth-check` |
| 版本 | `2.1.1` |
| 许可证 | `BSD-2-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add nth-check
```

```cp
import { NthRule, parseNth, nthCheck } from "nth-check";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `NthRule` | `class` | 字段: `a: i32, b: i32, valid: bool` · 方法: `matches` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `parseNth` | `(formula: string): NthRule` | 通过 `parseNth` 解析输入数据，返回 `NthRule`。 |
| `nthCheck` | `(formula: string, index: usize): bool` | 调用 `nthCheck` 并返回 `bool`。 |

## 最小调用示例

```cp
import { parseNth } from "nth-check";

pub fn main() {
    let result = parseNth("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
