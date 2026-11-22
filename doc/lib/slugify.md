# `cap:slugify` — URL 安全 Slug 生成器与规范校验器

> 参考来源：/data/dev/bun/src/string/immutable.cp | 吸收日期：2026-11-22

## 概述

`cap:slugify` 吸收自 Bun 不可变字符串处理工具集（`src/string/immutable.cp`），提供纯 CP、零外部依赖、零堆分配的 URL 安全 Slug 生成、词边界感知定长截断以及规范 Slug 合法性校验，适用于静态站点路由、文档锚点 ID、包名归一化与短链生成。

## 行为契约与边界规则

- **归一化折叠规则**：
  - ASCII 大写字母统一折叠为小写；
  - 英文撇号/单引号（`'`）自动省略连接（如 `"Don't Panic"` → `"dont-panic"` 而非 `"don-t-panic"`）；
  - 连续的非字母数字字符（空白、标点、符号）折叠为单个分隔符，并自动剥离首尾冗余分隔符，保证永不出现前导/尾随或双连字符（如 `"--"`）。
- **词边界感知有界截断（`slugifyBounded`）**：当生成结果超过 `max_len` 时，优先在最后一个分隔符处回退截断，避免将英文单词从中间生硬劈开，同时确保不留下尾随分隔符。
- **规范校验（`isValidSlug`）**：严格匹配 `[a-z0-9]+(-[a-z0-9]+)*`——拒绝空串、大写字母、前导/尾随连字符与连续 `--`。

## 导出 API

| 函数签名 | 说明 |
| :--- | :--- |
| `slugify(input, out): ([]const u8)?` | 生成标准 `'-'` 分隔的小写 ASCII Slug，缓冲不足返回 `null` |
| `slugifyWithSep(input, sep, out): ([]const u8)?` | 使用自定义分隔符（如 `'_'`）生成 Slug |
| `slugifyBounded(input, max_len, out): ([]const u8)?` | 在 `max_len` 字节上限内按词边界截断生成 Slug |
| `isValidSlug(s): bool` | 校验字符串是否严格符合规范 Slug 语法 `[a-z0-9]+(-[a-z0-9]+)*` |

## 边界注意事项

1. **纯符号输入**：仅包含标点或空白的输入会生成空切片 `""`（该结果不满足 `isValidSlug`）。
2. **输出缓冲防护**：`out` 长度不足以容纳生成结果时原子返回 `null`，绝不越界写入。

## 使用示例

```cp
import { slugify, slugifyBounded, isValidSlug } from "cap:slugify";

fn demo() {
    let b1: [64]u8 = undefined;
    let b2: [64]u8 = undefined;
    const s = slugify("Don't Panic: CP Lang v1.0!", b1[0..]).?; // "dont-panic-cp-lang-v1-0"
    const ok = isValidSlug(s);                                   // true
    const short_s = slugifyBounded("hello world from cp", 12, b2[0..]).?; // "hello-world"
    * = ok;
    * = short_s;
}
```
