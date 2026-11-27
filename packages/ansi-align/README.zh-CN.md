[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ansi-align

面向 CP 语言的零堆分配 ANSI 多行文本对齐库（支持左对齐、居中、右对齐），组合 `string-width` 与 `widest-line` 精确处理 ANSI 转义序列与 CJK 全角字符宽度。

对齐 npm 源包 [`ansi-align`](https://github.com/nexdrew/ansi-align)（ISC）。

## 安装与导入

```bash
cap add ansi-align
```

```cp
import {
    AlignMode,
    AlignOptions,
    ansiVisibleWidth,
    ansiMaxLineWidth,
    alignAnsi,
    alignAnsiWithOptions,
    alignAnsiLines,
    alignCenter,
    alignRight,
    alignLeft,
} from "ansi-align";
```

## 核心 API

### 类型与枚举

| 符号 | 定义 | 说明 |
| --- | --- | --- |
| `AlignMode` | `enum { Left, Center, Right }` | 每行应用的对齐模式。 |
| `AlignOptions` | `{ mode: AlignMode, split: string, pad: string }` | 对齐配置项，提供 `AlignOptions.default()`（默认 `Center`、`"\n"`、`" "`）。 |

### 函数列表

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `ansiVisibleWidth` | `(s: string): usize` | 计算字符串在终端的可见列宽（忽略 ANSI 转义序列并正确计入 CJK/emoji 宽度）。 |
| `ansiMaxLineWidth` | `(text: string, split: string): usize` | 计算按 `split` 分割后的所有行中的最大可见列宽。 |
| `alignAnsi` | `(text: string, mode: AlignMode, out: []u8): usize` | 使用默认换行与空格填充对齐多行文本并写入 `out`，返回写入字节数。 |
| `alignAnsiWithOptions` | `(text: string, opts: AlignOptions, out: []u8): usize` | 按自定义 `AlignOptions`（模式、分隔符、填充字符）对齐文本并写入 `out`。 |
| `alignAnsiLines` | `(lines: []const string, mode: AlignMode, pad: string, split: string, out: []u8): usize` | 对齐已拆分的字符串切片并以 `split` 拼接写入 `out`，返回写入字节数。 |
| `alignCenter` | `(text: string, out: []u8): usize` | `alignAnsi(text, AlignMode.Center, out)` 快捷方式。 |
| `alignRight` | `(text: string, out: []u8): usize` | `alignAnsi(text, AlignMode.Right, out)` 快捷方式。 |
| `alignLeft` | `(text: string, out: []u8): usize` | `alignAnsi(text, AlignMode.Left, out)` 快捷方式（不添加左侧填充）。 |

## 使用示例

### 1. 居中对齐带 ANSI 颜色的文本

```cp
import { print } from "cap:io";
import { alignCenter } from "ansi-align";

pub fn main() {
    let text = "\x1b[32mOK\x1b[0m\nLonger Status Line";
    let out: [256]u8 = undefined;
    let n = alignCenter(text, out[0..256]);
    print("{}\n", out[0..n]);
}
```

### 2. 自定义填充符与换行分隔符

```cp
import { print } from "cap:io";
import { AlignMode, AlignOptions, alignAnsiWithOptions } from "ansi-align";

pub fn main() {
    let opts = AlignOptions {
        mode: AlignMode.Right,
        split: "\r\n",
        pad: ".",
    };
    let out: [256]u8 = undefined;
    let n = alignAnsiWithOptions("hi\r\nhello world", opts, out[0..256]);
    print("{}\n", out[0..n]);
}
```

### 3. 对齐预拆分的字符串行切片

```cp
import { print } from "cap:io";
import { AlignMode, alignAnsiLines } from "ansi-align";

pub fn main() {
    let lines: [2]string = ["CP", "Zero-Alloc CLI"];
    let out: [128]u8 = undefined;
    let n = alignAnsiLines(lines[0..2], AlignMode.Center, " ", "\n", out[0..128]);
    print("{}\n", out[0..n]);
}
```

## 与源包差异与零拷贝安全亮点

- **零堆分配**：所有对齐结果直接写入调用方传入的 `out: []u8` 缓冲区并返回写入字节数 `usize`，不创建中间数组或临时字符串。
- **精确宽度计算**：底层复用 `string-width` 与 `widest-line`，兼容 ANSI SGR 色彩码、中文全角字符与组合字符。
- **只读借用**：输入文本与 `[]const string` 切片全程只读借用，安全无拷贝。
