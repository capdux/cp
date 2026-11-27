[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# filenamify

Zero-allocation cross-platform safe filename sanitizer (npm filenamify v6 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:filenamify` ([https://github.com/sindresorhus/filenamify](https://github.com/sindresorhus/filenamify))

| 元信息 | 值 |
| --- | --- |
| 包名 | `filenamify` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add filenamify
```

```cp
import {
    fnmIsInvalidChar,
    fnmHasInvalidChar,
    fnmToLowerAscii,
    fnmEq3IgnoreCase,
    fnmMatchAt,
    fnmEndsWith,
    fnmAppendStr,
    fnmUtf8SafeLen,
} from "filenamify";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `fnmIsInvalidChar` | `(ch: u8): bool` | 调用 `fnmIsInvalidChar` 并返回 `bool`。 |
| `fnmHasInvalidChar` | `(s: string): bool` | 调用 `fnmHasInvalidChar` 并返回 `bool`。 |
| `fnmToLowerAscii` | `(ch: u8): u8` | 调用 `fnmToLowerAscii` 并返回 `u8`。 |
| `fnmEq3IgnoreCase` | `(s: string, a: u8, b: u8, c: u8): bool` | 调用 `fnmEq3IgnoreCase` 并返回 `bool`。 |
| `fnmMatchAt` | `(buf: string, pos: usize, pat: string): bool` | 调用 `fnmMatchAt` 并返回 `bool`。 |
| `fnmEndsWith` | `(buf: string, len: usize, pat: string): bool` | 调用 `fnmEndsWith` 并返回 `bool`。 |
| `fnmAppendStr` | `(dst: []u8, pos: usize, src: string): usize` | 调用 `fnmAppendStr` 并返回 `usize`。 |
| `fnmUtf8SafeLen` | `(s: string, max_len: usize): usize` | 调用 `fnmUtf8SafeLen` 并返回 `usize`。 |
| `isReservedFilename` | `(name: string): bool` | 执行 `isReservedFilename` 条件判定，返回 `bool`。 |
| `filenamifyWithOptions` | `(input: string, replacement: string, max_len: usize, out: []u8): usize` | 调用 `filenamifyWithOptions` 并返回 `usize`。 |
| `filenamify` | `(input: string, out: []u8): usize` | 调用 `filenamify` 并返回 `usize`。 |

## 最小调用示例

```cp
import { isReservedFilename } from "filenamify";

pub fn main() {
    let result = isReservedFilename("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
