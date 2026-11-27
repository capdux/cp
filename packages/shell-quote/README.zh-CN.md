[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# shell-quote

Zero-heap POSIX shell argument quoting and command-line tokenization in pure CP (modeled after npm shell-quote)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:shell-quote` ([https://github.com/ljharb/shell-quote](https://github.com/ljharb/shell-quote))

| 元信息 | 值 |
| --- | --- |
| 包名 | `shell-quote` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add shell-quote
```

```cp
import {
    ParsedArgSpan,
    SplitResult,
    isSafeBareChar,
    needsQuoting,
    quoteArg,
    quoteArgs,
    splitCommand,
} from "shell-quote";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ParsedArgSpan` | `class` | 字段: `start: usize, len: usize` |
| `SplitResult` | `class` | 字段: `arg_count: usize, bytes_used: usize, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isSafeBareChar` | `(c: u8): bool` | 执行 `isSafeBareChar` 条件判定，返回 `bool`。 |
| `needsQuoting` | `(arg: &[]u8): bool` | 调用 `needsQuoting` 并返回 `bool`。 |
| `quoteArg` | `(arg: &[]u8, dst: []u8): i64` | 调用 `quoteArg` 并返回 `i64`。 |
| `quoteArgs` | `(args: &[]&[]u8, dst: []u8): i64` | 调用 `quoteArgs` 并返回 `i64`。 |
| `splitCommand` | `(cmd: &[]u8, out_buf: []u8, out_spans: []ParsedArgSpan): SplitResult` | 调用 `splitCommand` 并返回 `SplitResult`。 |

## 最小调用示例

```cp
import { isSafeBareChar } from "shell-quote";

pub fn main() {
    let result = isSafeBareChar(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
