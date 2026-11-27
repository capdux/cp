[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# indent-string

面向 CP 语言的零堆分配多行字符串缩进工具，对齐 npm [`indent-string`](https://github.com/sindresorhus/indent-string)（周下载 1.8 亿+）。

## 源包对齐说明

- **上游源包**：npm `indent-string`（`sindresorhus/indent-string`，MIT）
- **CP 实现**：`indent-string` v1.0.0，纯 CP 净室 MIT 实现
- **架构特点**：全程零堆分配，单趟逐行扫描写入调用方 `[]u8` 缓冲区，支持自定义缩进单元（空格、制表符、`"> "` 等）及是否缩进空行选项。

## 安装与导入

```bash
cap add indent-string
```

```cp
import {
    indentString,
    indentStringWith,
} from "indent-string";
```

## 核心 API

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `indentString` | `fn indentString(input: string, count: usize, out: []u8): usize` | 为 `input` 中所有非空行添加 `count` 个空格缩进并写入 `out`，返回写入字节数 |
| `indentStringWith` | `fn indentStringWith(input: string, count: usize, indent_unit: string, include_empty_lines: bool, out: []u8): usize` | 为每行添加 `count` 次 `indent_unit` 缩进；`include_empty_lines = false` 时跳过空行及纯空白行（`' '`/`'\t'`/`'\r'`） |

## 使用示例

### 1. 默认空格缩进

```cp
import { indentString } from "indent-string";
import { print } from "cap:io";

pub fn main() {
    let buf: [128]u8 = [0; 128];
    const n = indentString("Unicorns\nRainbows", 4, buf[0..128]);
    print(buf[0..n] as string);
    // "    Unicorns\n    Rainbows"
}
```

### 2. 自定义前缀并包含空行

```cp
import { indentStringWith } from "indent-string";

pub fn quoteBlock(text: string, out: []u8): usize {
    return indentStringWith(text, 1, "> ", true, out);
    // "line1\n\nline2" -> "> line1\n> \n> line2"
}
```

### 3. 制表符缩进并跳过空白行

```cp
import { indentStringWith } from "indent-string";

pub fn indentCode(code: string, out: []u8): usize {
    return indentStringWith(code, 1, "\t", false, out);
}
```

## 与源包差异与安全亮点

- **零堆分配**：无需正则替换或拆分为行数组，直接单趟写入调用方 `[]u8` 切片。
- **边界保护**：所有写入均严格校验 `p < out.len`，绝不发生缓冲区越界。
- **空行判定规则**：按 `\n` 切分，仅包含 `' '`、`'\t'`、`'\r'` 的行均视为空白行。
