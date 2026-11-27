[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# enquirer

Zero-heap pure CP interactive CLI prompt state machines for Confirm, Select, and MultiSelect (clean-room MIT implementation of npm enquirer v2)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:enquirer` ([https://github.com/enquirer/enquirer](https://github.com/enquirer/enquirer))

| 元信息 | 值 |
| --- | --- |
| 包名 | `enquirer` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add enquirer
```

```cp
import { ConfirmPrompt, SelectChoice, SelectPrompt, MultiSelectPrompt } from "enquirer";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ConfirmPrompt` | `class` | 字段: `message: string, initial: bool` · 方法: `new, parseAnswer, render` |
| `SelectChoice` | `class` | 字段: `label: string, enabled: bool` |
| `SelectPrompt` | `class` | 字段: `message: string, choices: [16]SelectChoice, choice_count: usize, cursor: usize, submitted: bool` · 方法: `new, addChoice, moveDown, moveUp, submit, render` |
| `MultiSelectPrompt` | `class` | 字段: `message: string, choices: [16]string, selected: [16]bool, choice_count: usize` · 方法: `new, addChoice, toggle, selectedCount, isSelected, render` |

## 最小调用示例

```cp
import { ConfirmPrompt } from "enquirer";

pub fn main() {
    // Refer to src/index.d.cp for ConfirmPrompt fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
