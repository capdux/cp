[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# mime_guess

Zero-allocation file path and extension MIME type guesser for CP (clean-room MIT, inspired by Rust mime_guess v2, powered by mime-types).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:mime_guess` ([https://github.com/abonander/mime_guess](https://github.com/abonander/mime_guess))（原库名 `mime_guess` → CP 包名 `mime_guess`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `mime_guess` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add mime_guess
```

```cp
import {
    MimeGuess,
    mgsStartsWith,
    mgsHasPathSep,
    mgsIsCompressibleMime,
    extractExt,
    getExtForMime,
    getCharsetForMime,
} from "mime_guess";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MimeGuess` | `class` | 字段: `mime: string, found: bool` · 方法: `fromExt, fromPath, first, firstOrOctetStream, firstOrTextPlain, isText, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mgsStartsWith` | `(s: string, prefix: string): bool` | 调用 `mgsStartsWith` 并返回 `bool`。 |
| `mgsHasPathSep` | `(s: string): bool` | 调用 `mgsHasPathSep` 并返回 `bool`。 |
| `mgsIsCompressibleMime` | `(mime_type: string): bool` | 调用 `mgsIsCompressibleMime` 并返回 `bool`。 |
| `extractExt` | `(path: string): string?` | 调用 `extractExt` 并返回 `string?`。 |
| `getExtForMime` | `(mime_type: string): string?` | 查询或读取目标数据，返回 `string?`。 |
| `getCharsetForMime` | `(mime_type: string): string?` | 查询或读取目标数据，返回 `string?`。 |

## 最小调用示例

```cp
import { getExtForMime } from "mime_guess";

pub fn main() {
    let result = getExtForMime("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
