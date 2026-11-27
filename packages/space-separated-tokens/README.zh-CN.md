[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# space-separated-tokens

面向 CP 语言的零堆分配 HTML/SVG 空格分隔与逗号分隔词元（token）列表解析、查询、归一化与序列化库。

对齐 npm 源包 [`space-separated-tokens`](https://github.com/wooorm/space-separated-tokens) `2.0.2`（MIT）。

## 安装与导入

```bash
cap add space-separated-tokens
```

```cp
import {
    parseSpaceTokens,
    parse,
    countTokens,
    tokenAt,
    hasSpaceToken,
    containsToken,
    stringifySpaceTokens,
    stringify,
    normalizeSpaceTokens,
    parseCommaTokens,
    stringifyCommaTokens,
} from "space-separated-tokens";
```

## 核心 API

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `parseSpaceTokens` / `parse` | `(input: string, out: []string): usize` | 按 ASCII 空白将 `input` 切分为零拷贝子切片写入 `out`，返回写入个数。 |
| `countTokens` | `(input: string): usize` | 统计 `input` 中空格分隔的 token 数量，无需数组缓冲。 |
| `tokenAt` | `(input: string, index: usize): string?` | 返回第 `index` 个 token 的零拷贝子切片，越界时返回 `null`。 |
| `hasSpaceToken` / `containsToken` | `(input: string, target: string): bool` | 检查 `input` 是否精确包含 `target` 词元（类似 HTML `classList.contains`）。 |
| `stringifySpaceTokens` / `stringify` | `(tokens: []const string, out: []u8): usize` | 将非空 token 以单个空格 `" "` 拼接写入 `out`，返回写入字节数。 |
| `normalizeSpaceTokens` | `(input: string, out: []u8): usize` | 去除首尾空白并将连续空白折叠为单个空格写入 `out`，返回写入字节数。 |
| `parseCommaTokens` | `(input: string, out: []string): usize` | 按逗号切分并去除两端空白写入 `out`，返回写入个数。 |
| `stringifyCommaTokens` | `(tokens: []const string, out: []u8): usize` | 以 `", "` 拼接 token 列表写入 `out`，返回写入字节数。 |

## 使用示例

### 1. 解析与查询 HTML `class` 属性

```cp
import { print } from "cap:io";
import { parse, hasSpaceToken, tokenAt } from "space-separated-tokens";

pub fn main() {
    let classes = "  btn   btn-primary\tactive  ";
    let tokens: [8]string = undefined;
    let n = parse(classes, tokens[0..8]);
    print("count={}, has_active={}\n", n, hasSpaceToken(classes, "active"));

    let second = tokenAt(classes, 1);
    if (second != null) {
        print("token[1]={}\n", second!); // "btn-primary"
    }
}
```

### 2. 空白归一化与序列化

```cp
import { print } from "cap:io";
import { normalizeSpaceTokens, stringify } from "space-separated-tokens";

pub fn main() {
    let out: [128]u8 = undefined;
    let n = normalizeSpaceTokens("  foo\n\tbar   baz  ", out[0..128]);
    print("normalized: '{}'\n", out[0..n]); // "foo bar baz"

    let list: [3]string = ["alpha", "beta", "gamma"];
    let m = stringify(list[0..3], out[0..128]);
    print("stringified: '{}'\n", out[0..m]); // "alpha beta gamma"
}
```

### 3. 逗号分隔词元处理

```cp
import { print } from "cap:io";
import { parseCommaTokens, stringifyCommaTokens } from "space-separated-tokens";

pub fn main() {
    let items: [8]string = undefined;
    let n = parseCommaTokens("a, b , c", items[0..8]);
    let buf: [64]u8 = undefined;
    let m = stringifyCommaTokens(items[0..n], buf[0..64]);
    print("csv: '{}'\n", buf[0..m]); // "a, b, c"
}
```

## 与源包差异与零拷贝安全亮点

- **零拷贝子切片**：`parse`、`parseCommaTokens` 与 `tokenAt` 直接返回指向原始 `input` 的子切片，零堆分配、零字符串复制。
- **无数组极速查询**：`countTokens`、`tokenAt`、`hasSpaceToken` 均以 O(1) 栈空间直接流式扫描。
- **附带逗号词元工具**：除完整覆盖 npm `space-separated-tokens` 外，额外提供 `parseCommaTokens` 与 `stringifyCommaTokens`。
