[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# escape-string-regexp

面向 CP 语言的零堆分配、单趟扫描正则表达式特殊字符转义与反转义库，对齐 npm [`escape-string-regexp`](https://github.com/sindresorhus/escape-string-regexp) v5（周下载 3.5 亿+）。

## 源包对齐说明

- **上游源包**：npm `escape-string-regexp` v5（`sindresorhus/escape-string-regexp`，MIT）
- **CP 实现**：`escape-string-regexp` v1.0.0，纯 CP 净室 MIT 实现
- **转义规则**：将 `| \ { } ( ) [ ] ^ $ + * ? .` 转义为 `\<ch>`，并将 `-` 转义为 `\x2d`（在字符类 `[...]` 内同样安全）。

## 安装与导入

```bash
cap add escape-string-regexp
```

```cp
import {
    esrIsSpecial,
    isRegexpMeta,
    hasRegexpMeta,
    needsRegexEscape,
    escapedRegexpLen,
    escapedLen,
    escapeStringRegexp,
    escapeStringRegexpChecked,
    escapeCharClass,
    unescapedLen,
    unescapeStringRegexp,
    unescapeRegexp,
} from "escape-string-regexp";
```

## 核心 API

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `escapeStringRegexp` | `fn escapeStringRegexp(input: string, out: []u8): usize` | 将正则元字符转义写入 `out`（`-` 转为 `\x2d`），返回写入字节数 |
| `escapeStringRegexpChecked` | `fn escapeStringRegexpChecked(input: string, out: []u8): usize?` | 安全检查版：若 `out.len < escapedRegexpLen(input)` 返回 `null`，不静默截断 |
| `escapedRegexpLen` | `fn escapedRegexpLen(input: string): usize` | 精确计算 `escapeStringRegexp` 所需的输出字节数 |
| `escapedLen` | `fn escapedLen(input: string): usize` | `escapedRegexpLen` 的别名 |
| `escapeCharClass` | `fn escapeCharClass(input: string, out: []u8): usize` | 仅转义正则字符类 `[...]` 特殊字符（`\`、`]`、`^` 及 `-` -> `\x2d`） |
| `unescapeStringRegexp` | `fn unescapeStringRegexp(escaped: string, out: []u8): usize` | 反转义 `escapeStringRegexp` 产物（`\x2d` -> `-`、`\<ch>` -> `<ch>`） |
| `unescapeRegexp` | `fn unescapeRegexp(escaped: string, out: []u8): usize` | `unescapeStringRegexp` 的别名 |
| `unescapedLen` | `fn unescapedLen(escaped: string): usize` | 精确计算 `unescapeStringRegexp` 的输出字节数 |
| `hasRegexpMeta` | `fn hasRegexpMeta(input: string): bool` | 判断字符串是否包含任意正则元字符或 `-`（可用于零拷贝快速跳过） |
| `needsRegexEscape` | `fn needsRegexEscape(input: string): bool` | `hasRegexpMeta` 的别名 |
| `isRegexpMeta` | `fn isRegexpMeta(c: u8): bool` | 判断单字节 `c` 是否为正则元字符或 `-` |
| `esrIsSpecial` | `fn esrIsSpecial(c: u8): bool` | 判断单字节 `c` 是否为 `| \ { } ( ) [ ] ^ $ + * ? .` 之一（不含 `-`） |

## 使用示例

### 1. 快速路径跳过 + 安全检查转义

```cp
import { hasRegexpMeta, escapeStringRegexpChecked } from "escape-string-regexp";
import { print } from "cap:io";

pub fn main() {
    const raw = "How much $ for a foo-bar?";
    if (!hasRegexpMeta(raw)) {
        print(raw);
        return;
    }
    let buf: [128]u8 = [0; 128];
    if (const n = escapeStringRegexpChecked(raw, buf[0..128])) {
        print(buf[0..n] as string);
        // "How much \$ for a foo\x2dbar\?"
    }
}
```

### 2. 字符类 `[...]` 专用转义

```cp
import { escapeCharClass } from "escape-string-regexp";

pub fn buildCharClass(chars: string, out: []u8): usize {
    return escapeCharClass(chars, out); // "a-z^]" -> "a\x2dz\^\]"
}
```

### 3. 转义与反转义往返

```cp
import { escapeStringRegexp, unescapeStringRegexp } from "escape-string-regexp";

pub fn roundTrip(input: string): usize {
    let esc: [256]u8 = [0; 256];
    let back: [256]u8 = [0; 256];
    const n1 = escapeStringRegexp(input, esc[0..256]);
    return unescapeStringRegexp(esc[0..n1] as string, back[0..256]);
}
```

## 与源包差异与安全亮点

- **零堆分配**：单趟扫描直接写入调用方 `[]u8` 缓冲区，无正则引擎回溯或临时字符串开销。
- **防截断安全接口**：`escapeStringRegexpChecked` 返回 `usize?`，缓冲区不足时直接返回 `null`，避免产生残缺转义序列。
- **双向与字符类扩展**：额外提供 `escapeCharClass`、`unescapeStringRegexp`、`hasRegexpMeta` 及精确长度预估函数。
