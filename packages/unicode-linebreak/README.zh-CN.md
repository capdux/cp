[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# unicode-linebreak

Zero-allocation pure CP UAX #14 Unicode line break opportunity detector (unicode-linebreak parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-linebreak` ([https://github.com/axelf4/unicode-linebreak](https://github.com/axelf4/unicode-linebreak))

| 元信息 | 值 |
| --- | --- |
| 包名 | `unicode-linebreak` |
| 版本 | `1.0.0` |
| 许可证 | `Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add unicode-linebreak
```

```cp
import {
    BreakKind,
    LineBreak,
    UlbrDecoded,
    ulbrDecodeUtf8,
    ulbrClassify,
    ulbrEvalBreak,
    findLineBreaks,
    countLineBreaks,
} from "unicode-linebreak";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `BreakKind` | `Mandatory, Allowed` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `LineBreak` | `class` | 字段: `offset: usize, kind: BreakKind` |
| `UlbrDecoded` | `class` | 字段: `cp: u32, len: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ulbrDecodeUtf8` | `(s: string, pos: usize): UlbrDecoded` | 调用 `ulbrDecodeUtf8` 并返回 `UlbrDecoded`。 |
| `ulbrClassify` | `(cp: u32): u8` | 调用 `ulbrClassify` 并返回 `u8`。 |
| `ulbrEvalBreak` | `(prev: u8, prevNonSp: u8, curr: u8): u8` | 调用 `ulbrEvalBreak` 并返回 `u8`。 |
| `findLineBreaks` | `(utf8: string, out: []LineBreak): usize` | 查询或读取目标数据，返回 `usize`。 |
| `countLineBreaks` | `(utf8: string): usize` | 调用 `countLineBreaks` 并返回 `usize`。 |

## 最小调用示例

```cp
import { ulbrDecodeUtf8 } from "unicode-linebreak";

pub fn main() {
    let result = ulbrDecodeUtf8("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
