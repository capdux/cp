[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# anstyle-parse

Zero-heap Paul Williams VT100/ANSI escape sequence state-machine parser (CSI/OSC/ESC/SGR) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:anstyle-parse` ([https://github.com/rust-cli/anstyle](https://github.com/rust-cli/anstyle)) / `crates.io:anstream` ([https://github.com/rust-cli/anstyle](https://github.com/rust-cli/anstyle))

| 元信息 | 值 |
| --- | --- |
| 包名 | `anstyle-parse` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add anstyle-parse
```

```cp
import {
    AnsiToken,
    SgrParams,
    nextToken,
    tokenizeAnsi,
    parseCsiParams,
    stripAnsiToBuf,
    hasAnsi,
    visibleByteLen,
} from "anstyle-parse";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `AnsiToken` | `class` | 字段: `kind: u8, start: usize, len: usize, final_byte: u8` |
| `SgrParams` | `class` | 字段: `params: [16]u16, count: usize, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `nextToken` | `(input: string, pos: usize): AnsiToken` | 调用 `nextToken` 并返回 `AnsiToken`。 |
| `tokenizeAnsi` | `(input: string, out: []AnsiToken): usize` | 调用 `tokenizeAnsi` 并返回 `usize`。 |
| `parseCsiParams` | `(csi_seq: string): SgrParams` | 通过 `parseCsiParams` 解析输入数据，返回 `SgrParams`。 |
| `stripAnsiToBuf` | `(input: string, out: []u8): usize` | 调用 `stripAnsiToBuf` 并返回 `usize`。 |
| `hasAnsi` | `(input: string): bool` | 执行 `hasAnsi` 条件判定，返回 `bool`。 |
| `visibleByteLen` | `(input: string): usize` | 调用 `visibleByteLen` 并返回 `usize`。 |

## 最小调用示例

```cp
import { parseCsiParams } from "anstyle-parse";

pub fn main() {
    let result = parseCsiParams("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
