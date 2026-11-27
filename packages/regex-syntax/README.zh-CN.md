[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# `regex-syntax`

CP 零堆分配正则表达式语法校验器、元字符转义器与字面量前缀提取器 —— 对齐 Rust [`regex-syntax`](https://github.com/rust-lang/regex)（v0.8）的纯 CP MIT 净室实现。

## 安装与导入

```bash
cap add regex-syntax
```

```cp
import {
    RSX_OK,
    RSX_ERR_UNCLOSED_GROUP,
    RSX_ERR_UNOPENED_GROUP,
    RSX_ERR_UNCLOSED_CLASS,
    RSX_ERR_INVALID_CLASS_RANGE,
    RSX_ERR_EMPTY_CLASS,
    RSX_ERR_DANGLING_QUANTIFIER,
    RSX_ERR_DOUBLE_QUANTIFIER,
    RSX_ERR_INVALID_QUANTIFIER,
    RSX_ERR_TRAILING_BACKSLASH,
    RSX_ERR_INVALID_ESCAPE,
    RSX_ERR_INVALID_GROUP_HEADER,
    RSX_ERR_NESTING_LIMIT,
    RegexSyntaxInfo,
    isMetaCharacter,
    escapeRegex,
    isWordByte,
    extractLiteralPrefix,
    validateRegexSyntax,
} from "regex-syntax";
```

## 核心 API

### 错误码常量 (`i32`)

| 常量 | 数值 | 含义 |
| :--- | :--- | :--- |
| `RSX_OK` | `0` | 正则语法合法 |
| `RSX_ERR_UNCLOSED_GROUP` / `RSX_ERR_UNOPENED_GROUP` | `1` / `2` | 未闭合的 `(` 或多余的 `)` |
| `RSX_ERR_UNCLOSED_CLASS` / `RSX_ERR_INVALID_CLASS_RANGE` / `RSX_ERR_EMPTY_CLASS` | `3` / `4` / `5` | 字符类未闭合、非法区间（如 `z-a`）或空字符类 `[]` |
| `RSX_ERR_DANGLING_QUANTIFIER` / `RSX_ERR_DOUBLE_QUANTIFIER` / `RSX_ERR_INVALID_QUANTIFIER` | `6` / `7` / `8` | 悬空量词、重复量词（`a**`）或非法 `{m,n}` |
| `RSX_ERR_TRAILING_BACKSLASH` / `RSX_ERR_INVALID_ESCAPE` | `9` / `10` | 末尾孤立 `\` 或非法转义序列 |
| `RSX_ERR_INVALID_GROUP_HEADER` / `RSX_ERR_NESTING_LIMIT` | `11` / `12` | 不合法的 `(?...)` 分组头或超出嵌套深度上限 |

### 类型与函数

| 符号 | 签名 / 字段 | 说明 |
| :--- | :--- | :--- |
| `RegexSyntaxInfo` | `valid: bool`, `err_code: i32`, `err_pos: usize`, `capture_count: usize`, `has_anchors: bool`, `has_backref: bool`, `min_len: usize`, `literal_prefix_len: usize`; `isOk(): bool` | 语法校验与静态模式分析结果 |
| `validateRegexSyntax` | `(pattern: string): RegexSyntaxInfo` | 单遍校验正则语法，统计捕获组数、锚点/反向引用、最小匹配长度与前缀长度 |
| `isMetaCharacter` | `(c: u8): bool` | 判断是否为正则元字符（严格对齐 `regex_syntax::is_meta_character`） |
| `escapeRegex` | `(input: string, out: []u8): usize` | 将所有元字符转义写入 `out`，返回写入字节数 |
| `isWordByte` | `(b: u8): bool` | 判断是否为 ASCII 单词字符（`[A-Za-z0-9_]`） |
| `extractLiteralPrefix` | `(pattern: string, out: []u8): usize` | 提取模式开头的必选字面量前缀（用于预过滤加速），返回字节数 |

## 代码示例

### 1. 校验用户输入的正则表达式并提取静态元信息

```cp
import { print } from "cap:io";
import { validateRegexSyntax, RSX_ERR_UNCLOSED_GROUP } from "regex-syntax";

fn main() {
    const info = validateRegexSyntax("^user_([0-9]{2,4})@example\\.com$");
    if (info.isOk()) {
        print("captures:", info.capture_count, "anchors:", info.has_anchors);
        print("min_len:", info.min_len, "prefix_len:", info.literal_prefix_len);
    }

    const bad = validateRegexSyntax("(abc[0-9]+");
    if (!bad.isOk() && bad.err_code == RSX_ERR_UNCLOSED_GROUP) {
        print("unclosed group at byte:", bad.err_pos);
    }
}
```

### 2. 元字符转义与必选字面量前缀提取

```cp
import { print } from "cap:io";
import { escapeRegex, extractLiteralPrefix } from "regex-syntax";

fn main() {
    let esc: [128]u8 = undefined;
    const n1 = escapeRegex("v1.2+beta (test)", esc[0..128]);
    print("escaped:", esc[0..n1]); // "v1\\.2\\+beta \\(test\\)"

    let pre: [64]u8 = undefined;
    const n2 = extractLiteralPrefix("https://[a-z]+\\.org", pre[0..64]);
    print("literal prefix:", pre[0..n2]); // "https://"
}
```

## 与源包差异及性能亮点

- **零堆分配**：无需构建堆上 `Ast`/`Hir` 节点树，单遍栈上扫描即可完成完整语法检查、嵌套深度防护（`RSX_ERR_NESTING_LIMIT`）与静态特征分析。
- **开箱即用预过滤**：`extractLiteralPrefix` 自动反转义前导字面量写入调用方缓冲，方便检索引擎先做高速子串过滤。
- **精确错误定位**：返回精确字节位置 `err_pos` 与 `RSX_ERR_*` 错误码，无异常开销。
