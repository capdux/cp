# cap:http_cache_control

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（RFC 9111 `Cache-Control` 规范），提供纯 CP、零外部依赖、零堆分配的 `Cache-Control` 指令解析、查询与规范化格式化器。

## 导入方式

```cp
import { CacheDirective, CacheControlIter, CacheControl } from "cap:http_cache_control";
```

## 核心类型与 API

### `CacheDirective` & `CacheControlIter`

- `CacheDirective`：`name: []const u8`、`value: ([]const u8)?`、`was_quoted: bool`。
- `CacheControlIter.init(header: []const u8)` / `next(*_): CacheDirective?`：按逗号流式切分指令，完整支持带引号值（如 `no-cache="Set-Cookie, Authorization"`）与转义引号。

### `CacheControl`

支持指令：`max_age`、`s_maxage`、`stale_while_revalidate`、`stale_if_error`、`min_fresh`、`max_stale`、`is_public`、`is_private`、`no_cache`、`no_store`、`no_transform`、`must_revalidate`、`proxy_revalidate`、`immutable`、`only_if_cached`。

- `CacheControl.parse(header: []const u8): CacheControl`：大小写不敏感解析指令名，严格校验 `delta-seconds`（防溢出、拒绝负数与非数字）。
- `isStorable(): bool`：`!no_store`。
- `isStorableInSharedCache(): bool`：`!no_store && !is_private`。
- `effectiveMaxAge(is_shared_cache: bool): u64?`：共享缓存优先取 `s_maxage`，遇到 `no_store` 或共享缓存下的 `private` 返回 `null`。
- `format(out: []u8): usize?`：零堆分配规范化输出，返回写入字节数；缓冲区不足返回 `null`。

## 使用示例

```cp
import { CacheControl } from "cap:http_cache_control";

fn example() {
    const cc = CacheControl.parse("public, max-age=60, s-maxage=600, stale-while-revalidate=30");
    * = cc.effectiveMaxAge(true);  // 600
    * = cc.effectiveMaxAge(false); // 60

    let buf: [128]u8 = undefined;
    if (n := cc.format(&buf)) {
        * = buf[0..n];
    }
}
```
