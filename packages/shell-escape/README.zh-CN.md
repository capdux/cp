[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# shell-escape

Zero-allocation POSIX sh and Windows CommandLineToArgvW argument escaper, command joiner, and env var name validator in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:shell-escape` ([https://github.com/xxorax/node-shell-escape](https://github.com/xxorax/node-shell-escape)) / `crates.io:shell-escape` ([https://github.com/sfackler/shell-escape](https://github.com/sfackler/shell-escape))

| 元信息 | 值 |
| --- | --- |
| 包名 | `shell-escape` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add shell-escape
```

```cp
import {
    isSafePosixByte,
    isSafePosixWord,
    escapePosix,
    needsWindowsQuotes,
    escapeWindows,
    joinPosixCommand,
    isValidEnvName,
} from "shell-escape";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isSafePosixByte` | `(b: u8): bool` | 执行 `isSafePosixByte` 条件判定，返回 `bool`。 |
| `isSafePosixWord` | `(arg: string): bool` | 执行 `isSafePosixWord` 条件判定，返回 `bool`。 |
| `escapePosix` | `(arg: string, out: []u8): usize` | 调用 `escapePosix` 并返回 `usize`。 |
| `needsWindowsQuotes` | `(arg: string): bool` | 调用 `needsWindowsQuotes` 并返回 `bool`。 |
| `escapeWindows` | `(arg: string, out: []u8): usize` | 调用 `escapeWindows` 并返回 `usize`。 |
| `joinPosixCommand` | `(args: &[]string, out: []u8): usize` | 调用 `joinPosixCommand` 并返回 `usize`。 |
| `isValidEnvName` | `(name: string): bool` | 执行 `isValidEnvName` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isSafePosixWord } from "shell-escape";

pub fn main() {
    let result = isSafePosixWord("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
