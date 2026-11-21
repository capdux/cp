# `cap:header_value_iter` — RFC 9110 HTTP 多值头部与参数迭代器

> 来源：吸收自 `/data/dev/bun/src/http/HeaderValueIterator.cp`

`cap:header_value_iter` 提供纯 CP、零外部依赖、零堆分配的 RFC 9110 HTTP 逗号/分号多值头部迭代器、参数解析器与 `Cache-Control` 指令解析器。

## 核心类型

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

- 直接基于 `HeaderValueIterator` 实现，大小写不敏感，支持带引号或不带引号的秒数参数（如 `max-age=3600`、`max-age="3600"`），自动忽略非法数值。

## 使用示例

```cp
import { HeaderValueIterator, HeaderParamIterator, CacheControl } from "cap:header_value_iter";

let it = HeaderValueIterator.init("gzip, , \"a, b\", br");
// 依次产出: "gzip", "\"a, b\"", "br"

const cc = CacheControl.parse("public, max-age=3600, s-maxage=86400, stale-while-revalidate=60, immutable");
// cc.is_public == true, cc.max_age.? == 3600, cc.immutable == true
```
