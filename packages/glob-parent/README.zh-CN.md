[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# glob-parent

Zero-allocation glob static parent directory extractor, glob detector, and escaper (glob-parent + is-glob parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:glob-parent` ([https://github.com/gulpjs/glob-parent](https://github.com/gulpjs/glob-parent))

| 元信息 | 值 |
| --- | --- |
| 包名 | `glob-parent` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add glob-parent
```

```cp
import {
    GlobSplit,
    findMatchingBracket,
    findMatchingBrace,
    findFirstGlobPos,
    stripLeadingNegation,
    trimTrailingSlashes,
    isGlob,
    globParent,
} from "glob-parent";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `GlobSplit` | `class` | 字段: `base: string, glob: string, is_glob: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `findMatchingBracket` | `(s: string, openIdx: usize): usize?` | 查询或读取目标数据，返回 `usize?`。 |
| `findMatchingBrace` | `(s: string, openIdx: usize): usize?` | 查询或读取目标数据，返回 `usize?`。 |
| `findFirstGlobPos` | `(s: string): usize?` | 查询或读取目标数据，返回 `usize?`。 |
| `stripLeadingNegation` | `(raw: string): string` | 调用 `stripLeadingNegation` 并返回 `string`。 |
| `trimTrailingSlashes` | `(s: string): string` | 调用 `trimTrailingSlashes` 并返回 `string`。 |
| `isGlob` | `(pattern: string): bool` | 执行 `isGlob` 条件判定，返回 `bool`。 |
| `globParent` | `(pattern: string): string` | 调用 `globParent` 并返回 `string`。 |
| `splitGlobBase` | `(pattern: string): GlobSplit` | 调用 `splitGlobBase` 并返回 `GlobSplit`。 |
| `isGlobSpecial` | `(c: u8): bool` | 执行 `isGlobSpecial` 条件判定，返回 `bool`。 |
| `escapeGlob` | `(raw: string, out: []u8): string` | 调用 `escapeGlob` 并返回 `string`。 |

## 最小调用示例

```cp
import { isGlob } from "glob-parent";

pub fn main() {
    let result = isGlob("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
