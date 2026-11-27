[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cli-truncate

Zero-allocation ANSI-aware CLI string truncator (end/start/middle, CJK double-width, space boundary preference) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cli-truncate` ([https://github.com/sindresorhus/cli-truncate](https://github.com/sindresorhus/cli-truncate))

| 元信息 | 值 |
| --- | --- |
| 包名 | `cli-truncate` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add cli-truncate
```

```cp
import {
    TruncatePosition,
    TruncateOptions,
    ctWriteBytes,
    ctSkipAnsi,
    ctUtf8Len,
    ctFindLastSpaceLe,
    ctIsSpaceAtCol,
    ctFindFirstSpaceGe,
} from "cli-truncate";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `TruncatePosition` | `END, START, MIDDLE` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TruncateOptions` | `class` | 字段: `position: TruncatePosition, prefer_truncation_on_space: bool, ellipsis: string` · 方法: `defaults` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ctWriteBytes` | `(out: []u8, pos: usize, src: string): usize` | 调用 `ctWriteBytes` 并返回 `usize`。 |
| `ctSkipAnsi` | `(text: string, pos: usize): usize` | 调用 `ctSkipAnsi` 并返回 `usize`。 |
| `ctUtf8Len` | `(text: string, pos: usize): usize` | 调用 `ctUtf8Len` 并返回 `usize`。 |
| `ctFindLastSpaceLe` | `(text: string, max_col: usize): usize` | 调用 `ctFindLastSpaceLe` 并返回 `usize`。 |
| `ctIsSpaceAtCol` | `(text: string, target_col: usize): bool` | 调用 `ctIsSpaceAtCol` 并返回 `bool`。 |
| `ctFindFirstSpaceGe` | `(text: string, min_col: usize, limit_col: usize): usize` | 调用 `ctFindFirstSpaceGe` 并返回 `usize`。 |
| `cliTruncateInto` | `(text: string, columns: usize, opts: TruncateOptions, out: []u8): usize` | 调用 `cliTruncateInto` 并返回 `usize`。 |
| `cliTruncateEndInto` | `(text: string, columns: usize, out: []u8): usize` | 调用 `cliTruncateEndInto` 并返回 `usize`。 |
| `cliTruncateStartInto` | `(text: string, columns: usize, out: []u8): usize` | 调用 `cliTruncateStartInto` 并返回 `usize`。 |
| `cliTruncateMiddleInto` | `(text: string, columns: usize, out: []u8): usize` | 调用 `cliTruncateMiddleInto` 并返回 `usize`。 |

## 最小调用示例

```cp
import { ctWriteBytes } from "cli-truncate";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ctWriteBytes(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
