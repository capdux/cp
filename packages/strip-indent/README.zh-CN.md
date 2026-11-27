[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# `strip-indent`

> **源包溯源**：`npm:strip-indent` ([sindresorhus/strip-indent](https://github.com/sindresorhus/strip-indent)) & `npm:min-indent` — 协议：MIT

纯 CP 实现的零堆分配多行公共缩进剥离器、制表符宽度感知的最小缩进分析器与逐行缩进工具。

## 零成本迁移对照（TS → CP）

直接沿用原包名 `strip-indent`，无需改名：

```typescript
// TypeScript / Node.js (npm: strip-indent)
import stripIndent from "strip-indent";
const clean = stripIndent("\n    foo\n      bar\n");
```

```cp
// CP（零堆分配，直接写入缓冲区）
import { stripIndent } from "strip-indent";

let buf: [128]u8 = undefined;
let n = stripIndent("\n    foo\n      bar\n", buf[0..128]);
let clean = buf[0..n] as string; // "\nfoo\n  bar\n"
```

## 安装配置

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "strip-indent": "1.0.0"
  }
}
```

## 核心 API 速览

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `StripIndentOptions` | `class { tab_width: usize, trim_blank_lines: bool, fn defaults(): StripIndentOptions }` | 制表符宽度与空白行清理配置 |
| `minIndent` | `(s: string): usize` | 计算所有非空行的最小前导空白长度 |
| `minIndentWithTabWidth` | `(s: string, tab_width: usize): usize` | 将 `\t` 按 `tab_width` 列宽计算最小缩进 |
| `stripIndent` | `(s: string, out: []u8): usize` | 剥离所有行的公共最小缩进 |
| `stripIndentInto` | `(s: string, out: []u8): usize` | `stripIndent` 别名 |
| `stripIndentBy` | `(s: string, n: usize, out: []u8): usize` | 每行最多剥离 `n` 个前导缩进字符 |
| `stripIndentWith` | `(s: string, opts: StripIndentOptions, out: []u8): usize` | 带 `tab_width` 与空白行裁剪的缩进剥离 |
| `indentLines` | `(s: string, count: usize, indent_char: u8, include_empty: bool, out: []u8): usize` | 为每行添加 `count` 个 `indent_char` 缩进 |
| `siIsIndentChar` / `siCopyRange` | `(ch: u8): bool` / `(out: []u8, pos: usize, s: string, start: usize, end: usize): usize` | 底层辅助函数 |

## 实战示例

### 1. 自动剥离公共缩进与定长剥离

```cp
import { minIndent, stripIndent, stripIndentBy } from "strip-indent";

let src = "    fn main() {\n        return;\n    }";
let m = minIndent(src); // 4

let out: [128]u8 = undefined;
let n1 = stripIndent(src, out[0..128]);       // 每行剥离 4 个空格
let n2 = stripIndentBy(src, 2, out[0..128]);  // 仅剥离 2 个空格
```

### 2. 制表符感知剥离与重新缩进

```cp
import { StripIndentOptions, minIndentWithTabWidth, stripIndentWith, indentLines } from "strip-indent";

let tabbed = "\tfoo\n\t  bar\n   \n\tbaz";
let cols = minIndentWithTabWidth(tabbed, 4); // 4

let opts = StripIndentOptions.defaults();
opts.tab_width = 4;
opts.trim_blank_lines = true;

let out: [256]u8 = undefined;
let n1 = stripIndentWith(tabbed, opts, out[0..256]);
let stripped = out[0..n1] as string;

// 为所有非空行重新添加 2 空格缩进
let out2: [256]u8 = undefined;
let n2 = indentLines(stripped, 2, ' ', false, out2[0..256]);
```

## 注意事项与边界说明

- **忽略纯空白行**：与 npm `strip-indent` / `min-indent` 行为一致，纯空白行不会拉低 `minIndent`。
- **保留换行符**：原样保留 `\n` 与 `\r\n`。
- **零堆分配**：所有输出均写入调用方 `[]u8` 缓冲区并返回实际写入字节数。
