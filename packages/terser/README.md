[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# terser

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:terser` ([https://github.com/terser/terser](https://github.com/terser/terser))（原库名 `terser` → CP 包名 `terser`）

Zero-allocation lexer-safe JS/TS/CSS code minifier for CP — clean-room MIT implementation inspired by npm `terser`.

纯 CP 零堆分配 JS/TS/CSS 词法安全代码压缩器（对标 npm `terser`），单遍状态机保护字符串与模板字面量、保留版权头注释、严防关键字粘连与 `a + +b` / `a - -b` 误合并。

## Install / 安装

```bash
cap add terser
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `MinifyOptions` | `struct { strip_comments, preserve_legal_comments, collapse_whitespace, keep_newlines }` + `.defaults()` | JS/TS minification configuration |
| `MinifyStats` | `struct { input_len, output_len, comments_removed, ok }` | Compression statistics & overflow status |
| `minifyJs` | `(input: string, opts: MinifyOptions, out: []u8): MinifyStats` | Lexer-safe JS/TS minifier |
| `minifyCss` | `(input: string, out: []u8): MinifyStats` | CSS minifier with comment stripping & trailing semicolon removal |

## Usage / 使用示例

```cp
import { MinifyOptions, minifyJs, minifyCss } from "terser";

let out: [256]u8 = undefined;
const stats = minifyJs("const x = 1; // comment\nreturn x + 2;", MinifyOptions.defaults(), out[0..256]);
// out[0..stats.output_len] == "const x=1;return x+2;"
```
