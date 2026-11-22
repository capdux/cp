# `cap:nfa_regex` — Thompson NFA 线性时间无回溯正则表达式标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:text`，导入请使用 `import { ... } from "cap:text"`。

## 1. 概述
`cap:nfa_regex`（动态挂载 `cap.nfa_regex`）是纯 CP 实现的 Thompson NFA 线性时间 `O(N * M)` 正则表达式引擎，彻底免疫 ReDoS 灾难性回溯。支持 `.`、`*`、`+`、`?`、`|`、分组 `(...)`、`^`/`$` 锚点、字符类 `[a-z0-9]`/`[^...]` 及 `\d`/`\w`/`\s` 等转义序列。

## 2. 核心类型
- `RegexError`：`EmptyPattern`、`UnmatchedParen`、`UnclosedCharClass`、`TrailingEscape`、`NothingToRepeat`、`InvalidRange`、`PatternTooComplex`、`OutOfMemory`
- `MatchSpan`：`start: usize`、`end: usize`
- `NfaRegex`

## 3. API 速查
- `NfaRegex.compile(alloc, pattern)` -> `NfaRegex`
- `re.isMatch(alloc, input)` -> `bool`
- `re.findFirst(alloc, input)` -> `MatchSpan?`
- `re.findAll(alloc, input)` -> `[]MatchSpan`
- `re.deinit()`

## 4. 示例
```cp
import { NfaRegex } from "cap:text";
```
