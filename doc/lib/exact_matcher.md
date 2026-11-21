# `cap:exact_matcher` — 定长整型打包 $O(1)$ 字符串快速匹配器

> 吸收自 `/data/dev/bun/src/string/immutable/exact_size_matcher.cp`，将 1 / 2 / 4 / 8 / 12 / 16 字节以内的短字符串打包为单个无符号整数，用于编译期 `switch` 无分支 $O(1)$ 关键字/HTTP 方法/协议标签分发，并修正了等长 `matchLower` 未转小写的问题。

## 核心导出 API

| 符号 | 说明 |
| :--- | :--- |
| `ExactSizeMatcher(max_bytes)` | 泛型定长匹配器（支持 `1, 2, 4, 8, 12, 16` 字节），提供 `match(str)`、`matchLower(str)`、`case(comptime str)` |
| `Matcher4` / `Matcher8` / `Matcher16` | 常用 4 / 8 / 16 字节特化别名（底层 `u32` / `u64` / `u128`） |
| `packU32` / `packU32Lower` | 4 字节大小写敏感/不敏感整型打包 |
| `packU64` / `packU64Lower` | 8 字节大小写敏感/不敏感整型打包 |
| `packU128` / `packU128Lower` | 16 字节大小写敏感/不敏感整型打包 |

## 示例

```cp
import { Matcher8 } from "cap:exact_matcher";

const method = "DELETE";
const is_delete = switch (Matcher8.match(method)) {
    Matcher8.case("DELETE") => true,
    * => false,
};
```
