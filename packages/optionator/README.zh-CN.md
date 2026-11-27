[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# optionator

Zero-heap CLI option specification and auto-aligned --help documentation generator in pure CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `optionator` |
| 版本 | `0.9.4` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add optionator
```

```cp
import {
    OptionSpec,
    optnLeftColLen,
    formatOptionRow,
    generateHelp,
    findOptionSpec,
} from "optionator";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `OptionSpec` | `class` | 字段: `long_name: string, short_alias: string, value_type: string, description: string, default_val: string, required: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `optnLeftColLen` | `(spec: OptionSpec): usize` | 调用 `optnLeftColLen` 并返回 `usize`。 |
| `formatOptionRow` | `(spec: OptionSpec, col_width: usize, out: []u8): usize` | 通过 `formatOptionRow` 编码或格式化输出，返回 `usize`。 |
| `generateHelp` | `(program: string, summary: string, specs: &[]OptionSpec, out: []u8): usize` | 调用 `generateHelp` 并返回 `usize`。 |
| `findOptionSpec` | `(specs: &[]OptionSpec, flag: string): OptionSpec?` | 查询或读取目标数据，返回 `OptionSpec?`。 |

## 最小调用示例

```cp
import { findOptionSpec } from "optionator";

pub fn main() {
    let result = findOptionSpec(/* specs: &[]OptionSpec */, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
