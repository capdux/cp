[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# regex-syntax

面向 CP 的零堆分配正则表达式语法校验器、元字符转义器与静态字面量前缀提取器（对齐 Rust `regex-syntax` v0.8）。

全程栈上解析与校验（分组配对、字符类区间、转义序列、`{m,n}` 量词、嵌套深度保护），精准输出错误码与字节偏移、捕获组计数、锚点/反向引用标志、最小匹配长度，并提取静态字面量前缀以供 SIMD 预过滤。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源生态平台 | **crates.io** |
| 原包名与版本 | `regex-syntax` v0.8（CP 生态版 `v1.0.0`） |
| 上游仓库 | `https://github.com/rust-lang/regex` |
| 开源协议 | MIT（上游：MIT OR Apache-2.0） |

从 Rust 迁移至 CP 保持原包名，零成本切换：

```rust
// Rust (crates.io: regex-syntax)
let escaped = regex_syntax::escape("a.b+c?");
let is_meta = regex_syntax::is_meta_character(b'*');
let hir = regex_syntax::parse(pattern)?;
```

```cp
// CP (cap: regex-syntax) — 零堆分配栈上校验与转义
import { escapeRegex, isMetaCharacter, validateRegexSyntax, extractLiteralPrefix } from "regex-syntax";
const n = escapeRegex("a.b+c?", outBuf);
const info = validateRegexSyntax(pattern);
```

## 安装

```bash
cap add regex-syntax
```

## 实战示例

### 基础用法：语法校验、元字符转义与字面量前缀提取

```cp
import {
    isMetaCharacter,
    escapeRegex,
    isWordByte,
    validateRegexSyntax,
    extractLiteralPrefix
} from "regex-syntax";

// 1. 校验正则语法并分析静态元信息
const info = validateRegexSyntax("^GET\\s+(/api/[a-z]+)");
if (info.isOk()) {
    // info.capture_count == 1
    // info.has_anchors == true
    // info.has_backref == false
    // info.min_len >= 9
}

// 2. 将用户输入中的正则元字符安全转义到栈缓冲区
let escBuf: [64]u8 = undefined;
const escLen = escapeRegex("price=$9.99 (USD)", escBuf[0..64]);
// escBuf[0..escLen] == "price=\\$9\\.99 \\(USD\\)"

// 3. 提取静态字面量前缀（可配合 memchr/memmem 做高速预过滤）
let pfxBuf: [64]u8 = undefined;
const pfxLen = extractLiteralPrefix("https://api\\.example\\.com/[0-9]+", pfxBuf[0..64]);
// pfxBuf[0..pfxLen] == "https://api.example.com/"

const meta = isMetaCharacter('*' as u8); // true
const word = isWordByte('_' as u8);      // true
```

### 安全与边界用法：异常正则精准诊断与嵌套深度防护

```cp
import { validateRegexSyntax } from "regex-syntax";

// 1. 精准定位未闭合分组与悬空量词的错误码和字节位置
const bad1 = validateRegexSyntax("(abc");
// bad1.isOk() == false, bad1.err_code == 1 (UNCLOSED_GROUP), bad1.err_pos == 4

const bad2 = validateRegexSyntax("*abc");
// bad2.isOk() == false, bad2.err_code == 6 (DANGLING_QUANTIFIER), bad2.err_pos == 0

// 2. 拦截非法字符类区间（[z-a]）与空字符类（[]）
const bad3 = validateRegexSyntax("[z-a]");
// bad3.isOk() == false, bad3.err_code == 4 (INVALID_CLASS_RANGE)

// 3. 内置嵌套深度上限防护（err_code == 12），防止恶意超深括号引发栈溢出
```

## API 速查表

| 导出符号 | 签名 | 说明 |
| --- | --- | --- |
| `RegexSyntaxInfo` | `class { valid, err_code, err_pos, capture_count, has_anchors, has_backref, min_len, literal_prefix_len, fn isOk(): bool }` | 语法校验与静态属性结果结构。 |
| `validateRegexSyntax` | `fn(pattern: string): RegexSyntaxInfo` | 校验正则语法并统计捕获组、锚点、反向引用、最小长度与前缀长度。 |
| `isMetaCharacter` | `fn(c: u8): bool` | 判断是否为 18 个正则元字符（`\.+*?()\|[]{}^$#&-~`）。 |
| `escapeRegex` | `fn(input: string, out: []u8): usize` | 转义元字符并写入 `out`，返回写入字节数。 |
| `isWordByte` | `fn(b: u8): bool` | 判断是否为 ASCII 词字符 `[A-Za-z0-9_]`。 |
| `extractLiteralPrefix` | `fn(pattern: string, out: []u8): usize` | 提取开头的静态字面量前缀写入 `out`，返回字节数。 |

## 错误码速查（`RegexSyntaxInfo.err_code`）

`0`: 正常 · `1`: 未闭合分组 · `2`: 多余右括号 · `3`: 未闭合字符类 · `4`: 非法字符类区间 · `5`: 空字符类 · `6`: 悬空量词 · `7`: 重复量词 · `8`: 非法量词 · `9`: 末尾孤立反斜杠 · `10`: 非法转义 · `11`: 非法分组头 · `12`: 超出嵌套深度上限。

## 协议

MIT — 对齐 Rust `regex-syntax` v0.8 的纯 CP 净室实现。
