# CP 标准库：HTTP 缓存语义与条件请求协商手册 (`cap:http_cache`)

## 一、 架构总览与设计准则

`cap:http_cache` 是 CP 语言官方内置的纯 CP **RFC 9111 HTTP 缓存语义与 RFC 9110 条件请求协商引擎**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为 CP Web 框架、反向代理/CDN 网关与 HTTP 客户端缓存（配合 `cap:http` / `cap:cache`）打造。既可通过 `import { ... } from "cap:http_cache"` 导入，也支持 `import cap from "cap"; cap.http_cache.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/http_cache/cache_policy.cp` | `// Source: cp/lib/http_cache/cache_policy.cp (...)` | `HttpCacheError`、`FreshnessStatus`、`CacheHeader`、`CacheControl`、`EntityTag`、`HttpCacheEvaluator` |
| `lib/http_cache/index.cp` | `// Source: cp/lib/http_cache/index.cp (...)` | `cap:http_cache` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与基础类型

- `HttpCacheError`：
  - `InvalidETag`：ETag 缺少双引号、弱前缀非严格 `W/`（如小写 `w/`）、包含控制字符/空格/未闭合引号，或列表中存在非法项。
  - `InvalidCacheControl`：指令名非法、数值指令缺少非负整数参数或溢出 `u64`、纯布尔指令误带 `=`、或引号未闭合。
  - `InvalidHeaderValue`：`Vary` 头字段包含非法 HTTP token 字符。
  - `OutOfMemory`：底层缓冲区分配失败。
- `FreshnessStatus`：
  - `Fresh`：缓存响应在新鲜度寿命内（或满足 `max-stale` 条件），可直接命中返回。
  - `Stale`：缓存响应已过期或被 `no-cache` / `min-fresh` 要求重新向源站验证。
  - `Uncacheable`：命中 `no-store` 或共享缓存下的 `private`，严禁存储。
- `CacheHeader`：
  - `name: []const u8`
  - `value: []const u8`

### 2. `CacheControl`（RFC 9111 §5.2 指令解析与序列化）

- **核心字段**（均带安全默认值）：
  - `no_store: bool = false`、`no_cache: bool = false`
  - `public_flag: bool = false`、`private_flag: bool = false`
  - `must_revalidate: bool = false`、`immutable: bool = false`
  - `max_age: u64? = null`、`s_maxage: u64? = null`
  - `stale_while_revalidate: u64? = null`
  - `min_fresh: u64? = null`、`max_stale: u64? = null`（无参 `max-stale` 解析为 `std.math.maxInt(u64)`）
- **方法**：
  - `CacheControl.parse(header_val): CacheControl ! HttpCacheError`：大小写不敏感解析逗号分隔指令，支持引号参数值与未知扩展指令平滑忽略。
  - `toHeader(alloc): ([]u8) ! HttpCacheError`：按确定规范顺序序列化为 `Cache-Control` 头字符串，调用方通过 `alloc.free` 释放。

### 3. `EntityTag`（RFC 9110 §8.8.3 强/弱 ETag 与条件协商）

- **核心字段**：
  - `weak: bool = false`（仅严格 `W/` 前缀置为 `true`）
  - `opaque_tag: []const u8`（零拷贝切片，不含外层双引号）
- **方法**：
  - `EntityTag.parse(raw): EntityTag ! HttpCacheError`：校验 `W/"..."` 与 `"..."` 格式及 RFC 9110 `etagc` 字符集。
  - `EntityTag.strongEquals(a, b): bool`：两者均非弱标签且不透明串相等时返回 `true`。
  - `EntityTag.weakEquals(a, b): bool`：忽略 `weak` 标志，仅比较不透明串。
  - `EntityTag.matchesIfNoneMatch(header_val, current_etag): bool ! HttpCacheError`：支持 `*` 与引号感知逗号列表，使用**弱比较**判断是否命中 `304 Not Modified`。
  - `EntityTag.matchesIfMatch(header_val, current_etag): bool ! HttpCacheError`：支持 `*` 与引号感知逗号列表，使用**强比较**判断是否满足前置条件（返回 `false` 触发 `412 Precondition Failed`，有效防止弱 ETag 并发覆盖）。

### 4. `HttpCacheEvaluator`（新鲜度评估与 `Vary` 缓存键）

- `HttpCacheEvaluator.evaluateResponse(req_cc, resp_cc, is_shared_cache, age_sec, expires_minus_date_sec): FreshnessStatus`
  - 优先拦截 `no-store` 与共享缓存下的 `private`（返回 `.Uncacheable`）。
  - 检查请求或响应 `no-cache`（返回 `.Stale`）。
  - 按 RFC 9111 优先级计算新鲜度寿命：`s-maxage`（仅共享缓存） > `max-age` > `Expires - Date`（负值归零）。
  - 结合 `age_sec`、`min-fresh` 与 `max-stale` 判定 `.Fresh` 或 `.Stale`。
- `HttpCacheEvaluator.isModifiedSince(last_modified_sec, if_modified_since_sec): bool`
  - RFC 9110 `If-Modified-Since` 时间比较辅助方法（返回 `false` 表示未修改，可返回 `304`）。
- `HttpCacheEvaluator.buildVaryKey(alloc, method, url, vary_header, req_headers): ([]u8)? ! HttpCacheError`
  - 遇到 `Vary: *` 返回 `null`（不可共享缓存）。
  - 对 `Vary` 头名称去重、转小写并字典序排序，大小写不敏感提取请求头（同名多值按序合并），构造确定性缓存键；非 `null` 返回值由调用方通过 `alloc.free` 释放。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { CacheControl, EntityTag, HttpCacheEvaluator, CacheHeader } from "cap:http_cache";
import cap from "cap";

pub fn demoHttpCache(): ! {
    const alloc = std.heap.page_allocator;

    const req_cc = CacheControl.parse("min-fresh=30")?;
    const resp_cc = CacheControl.parse("public, max-age=60, s-maxage=300")?;
    const status = HttpCacheEvaluator.evaluateResponse(req_cc, resp_cc, true, 120, null);
    * = status; // .Fresh（共享缓存优先使用 s-maxage=300，剩余 180 >= 30）

    const etag = EntityTag.parse("\"v42\"")?;
    const hit_304 = EntityTag.matchesIfNoneMatch("W/\"v42\", \"v41\"", etag)?;
    const ok_412 = EntityTag.matchesIfMatch("W/\"v42\"", etag)?; // false（强比较拒绝弱 ETag）
    * = hit_304;
    * = ok_412;

    const hdrs: []const CacheHeader = [
        CacheHeader{ name: "Accept-Encoding", value: "gzip, br" },
    ];
    if (key := cap.http_cache.HttpCacheEvaluator.buildVaryKey(alloc, "GET", "/api/data", "Accept-Encoding", hdrs)?) {
        defer alloc.free(key);
    }
}
```
