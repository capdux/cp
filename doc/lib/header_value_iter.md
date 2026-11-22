# `cap:header_value_iter` — RFC 9110 HTTP 多值头部与参数迭代器

> 参考来源：/data/dev/bun/src/http/HeaderValueIterator.cp | 吸收日期：2026-11-22

## 概述

`cap:header_value_iter` 吸收自 Bun HTTP 核心模块（`src/http/HeaderValueIterator.cp`），提供纯 CP、零外部依赖、零堆分配的 RFC 9110 HTTP 逗号/分号多值头部迭代器、参数解析器与 `Cache-Control` 指令解析器。

## 上游行为契约与 CP 增强

- **Bun 上游设计（`HeaderValueIterator.cp`）**：Bun 原生实现对输入首尾裁剪 OWS（SP `0x20` 与 HTAB `0x09`），按 `,` 分词并在 `next()` 中自动跳过空元素（处理形如 `"a, , , b"` 的合法 RFC 9110 空列表项）。
- **RFC 9110 引号字符串保护（CP 生产增强）**：标准库在此基础上升级为单遍扫描状态机，严格保护双引号字符串 `"..."` 内部的逗号与 `\"` 转义序列，避免将 `ETag`、`Link`、`Cache-Control` 等带引号参数中的逗号误切分。
- **分号参数与 `Cache-Control` 扩展**：在同一零拷贝扫描范式下扩展了 `HeaderParamIterator`（处理 `Content-Disposition` / `Set-Cookie` / `Content-Type` 的 `; key=value` 与无值标志）以及大小写不敏感的 `CacheControl` 结构体解析器。

## 核心类型与 API

### `HeaderValueIterator`

```cp
pub struct HeaderValueIterator {
    input: []const u8,
    pos: usize = 0,

    static fn init(input: []const u8): HeaderValueIterator
    fn next(*_): ([]const u8)?
}
```

- 按逗号 `,` 切分多值 HTTP 头部，自动裁剪首尾 OWS（空格与 Tab）、跳过空元素，并严格保护引号字符串 `"..."` 内部的逗号与 `\"` 转义。
- 所有返回切片均直接借用自 `input`，零内存拷贝、零堆分配。

### `HeaderParam` & `HeaderParamIterator`

```cp
pub struct HeaderParam {
    key: []const u8 = "",
    value: []const u8 = "",
    was_quoted: bool = false,
}

pub struct HeaderParamIterator {
    input: []const u8,
    pos: usize = 0,

    static fn init(input: []const u8): HeaderParamIterator
    fn next(*_): HeaderParam?
}
```

- 按分号 `;` 切分 HTTP 头部参数（如 `Content-Disposition`、`Set-Cookie`、`Content-Type`），支持 `key=value`、`key="quoted; value"`（自动剥离外层引号并置 `was_quoted = true`）以及无值标志参数（如 `HttpOnly`、`Secure`）。

### `CacheControl`

```cp
pub struct CacheControl {
    no_cache: bool = false,
    no_store: bool = false,
    no_transform: bool = false,
    must_revalidate: bool = false,
    is_public: bool = false,
    is_private: bool = false,
    immutable: bool = false,
    max_age: u32? = null,
    s_maxage: u32? = null,
    stale_while_revalidate: u32? = null,

    static fn parse(header: []const u8): CacheControl
}
```

- 直接基于 `HeaderValueIterator` 实现，指令名大小写不敏感，支持带引号或不带引号的秒数参数（如 `max-age=3600`、`max-age="3600"`），自动忽略负数、溢出或非数字的畸形数值。

## 边界注意事项

1. **切片生命周期**：`HeaderValueIterator.next()` 与 `HeaderParamIterator.next()` 返回的切片指向原始 `input` 缓冲区，调用方需确保在消费结果期间 `input` 保持有效。
2. **未闭合引号容错**：若输入包含未闭合的双引号，扫描器会安全推进至输入末尾而不会死循环或越界。
3. **空值与纯空白输入**：空字符串或全由 `,`、`;` 与 OWS 组成的输入会直接返回 `null`（`CacheControl.parse` 返回全默认结构体）。

## 使用示例

```cp
import { HeaderValueIterator, HeaderParamIterator, CacheControl } from "cap:header_value_iter";

let it = HeaderValueIterator.init("gzip, , \"a, b\", br");
// 依次产出: "gzip", "\"a, b\"", "br"
while (item := it.next()) {
    * = item;
}

let pit = HeaderParamIterator.init("attachment; filename=\"report;2026.pdf\"; Secure");
// 依次产出: { key: "attachment", value: "" }, { key: "filename", value: "report;2026.pdf", was_quoted: true }, { key: "Secure", value: "" }
while (p := pit.next()) {
    * = p;
}

const cc = CacheControl.parse("public, max-age=3600, s-maxage=86400, stale-while-revalidate=60, immutable");
// cc.is_public == true, cc.max_age.? == 3600, cc.immutable == true
* = cc;
```
