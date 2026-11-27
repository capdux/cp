[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# stack-utils

Zero-heap V8 stack trace line parser extracting function name, file path, line, column, and native flag

| 元信息 | 值 |
| --- | --- |
| 包名 | `stack-utils` |
| 版本 | `2.0.6` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add stack-utils
```

```cp
import { StackFrame, stkuInvalid, stkuParseLoc, parseStackLine } from "stack-utils";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `StackFrame` | `class` | 字段: `function_name: string, file: string, line: u32, column: u32, is_native: bool, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `stkuInvalid` | `(): StackFrame` | 调用 `stkuInvalid` 并返回 `StackFrame`。 |
| `stkuParseLoc` | `(fn_name: string, loc: string): StackFrame` | 调用 `stkuParseLoc` 并返回 `StackFrame`。 |
| `parseStackLine` | `(line: string): StackFrame` | 通过 `parseStackLine` 解析输入数据，返回 `StackFrame`。 |

## 最小调用示例

```cp
import { parseStackLine } from "stack-utils";

pub fn main() {
    let result = parseStackLine("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
