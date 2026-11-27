[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# strip-final-newline

Zero-copy final LF/CRLF newline stripper and normalizer matching npm strip-final-newline (250M+/wk) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:strip-final-newline` ([https://github.com/sindresorhus/strip-final-newline](https://github.com/sindresorhus/strip-final-newline))

| 元信息 | 值 |
| --- | --- |
| 包名 | `strip-final-newline` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add strip-final-newline
```

```cp
import {
    stripFinalNewline,
    stripAllTrailingNewlines,
    hasFinalNewline,
    hasFinalCrlf,
    ensureFinalNewline,
} from "strip-final-newline";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `stripFinalNewline` | `(input: string): string` | 调用 `stripFinalNewline` 并返回 `string`。 |
| `stripAllTrailingNewlines` | `(input: string): string` | 调用 `stripAllTrailingNewlines` 并返回 `string`。 |
| `hasFinalNewline` | `(input: string): bool` | 执行 `hasFinalNewline` 条件判定，返回 `bool`。 |
| `hasFinalCrlf` | `(input: string): bool` | 执行 `hasFinalCrlf` 条件判定，返回 `bool`。 |
| `ensureFinalNewline` | `(input: string, use_crlf: bool, out: []u8): usize` | 调用 `ensureFinalNewline` 并返回 `usize`。 |

## 最小调用示例

```cp
import { stripFinalNewline } from "strip-final-newline";

pub fn main() {
    let result = stripFinalNewline("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
