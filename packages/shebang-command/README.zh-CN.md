[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# shebang-command

Zero-allocation shebang executable command extractor reusing shebang-regex for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:shebang-command` ([https://github.com/kevva/shebang-command](https://github.com/kevva/shebang-command))

| 元信息 | 值 |
| --- | --- |
| 包名 | `shebang-command` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add shebang-command
```

```cp
import {
    ShebangCmd,
    Parts,
    skipSpaces,
    scanTokenEnd,
    trimRight,
    basenameStart,
    locateParts,
    shebangCommand,
} from "shebang-command";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ShebangCmd` | `class` | 字段: `ok: bool, bin_len: usize, arg_len: usize` |
| `Parts` | `class` | 字段: `ok: bool, bin_start: usize, bin_len: usize, arg_start: usize, arg_len: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `skipSpaces` | `(s: string, pos: usize, end: usize): usize` | 调用 `skipSpaces` 并返回 `usize`。 |
| `scanTokenEnd` | `(s: string, pos: usize, end: usize): usize` | 调用 `scanTokenEnd` 并返回 `usize`。 |
| `trimRight` | `(s: string, start: usize, end: usize): usize` | 调用 `trimRight` 并返回 `usize`。 |
| `basenameStart` | `(s: string, start: usize, end: usize): usize` | 调用 `basenameStart` 并返回 `usize`。 |
| `locateParts` | `(input: string): Parts` | 调用 `locateParts` 并返回 `Parts`。 |
| `shebangCommand` | `(input: string, out_cmd: []u8): usize` | 调用 `shebangCommand` 并返回 `usize`。 |
| `parseShebangCmd` | `(input: string, out_bin: []u8, out_arg: []u8): ShebangCmd` | 通过 `parseShebangCmd` 解析输入数据，返回 `ShebangCmd`。 |

## 最小调用示例

```cp
import { parseShebangCmd } from "shebang-command";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = parseShebangCmd("example", out_buf[0..256], buf_1[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
