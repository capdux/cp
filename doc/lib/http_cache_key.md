# cap:http_cache_key

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（HTTP Cache Key Normalizer & Builder），提供纯 CP、零外部依赖、零堆分配的 HTTP 缓存键规范化构造与 Vary 维度追加库。

## 导入方式

```cp
import {
    isTrackingQueryParam,
    normalizeQueryWithoutTracking,
    buildCacheKey,
    appendVaryDimension,
} from "cap:http_cache_key";
```

## 核心 API

- `isTrackingQueryParam(key: []const u8): bool`：大小写不敏感识别常见追踪查询参数（`utm_*`、`fbclid`、`gclid`、`mc_cid`、`mc_eid`、`ref_src`）。
- `normalizeQueryWithoutTracking(query: []const u8, out: []u8): ([]const u8)?`：去除首部 `?`，过滤追踪参数与空段，按原序输出剩余查询参数。
- `buildCacheKey(method, scheme, host, path, query, strip_tracking, out): ([]const u8)?`：
  - `method` 转大写，`scheme` / `host` 转小写；
  - 剥离协议默认端口（`http` `:80`、`https` `:443`），保留非默认与跨协议端口；
  - 空路径归约为 `/`，非 `/` 开头路径自动补 `/`；
  - 按 `strip_tracking` 决定是否剥离追踪参数与空段；
  - 输出格式如 `"GET https://example.com/api?id=1"`。
- `appendVaryDimension(out: []u8, pos: *usize, header_name: []const u8, header_val: []const u8): bool`：以 `\n<lower_name>:<trimmed_val>` 格式原子追加 Vary 维度至缓存键尾部（防换行注入，缓冲不足时保持 `pos` 不变）。

## 使用示例

```cp
import { buildCacheKey, appendVaryDimension } from "cap:http_cache_key";

fn example() {
    let buf: [256]u8 = undefined;
    if (base := buildCacheKey("get", "HTTPS", "Example.COM:443", "/api", "?id=1&utm_source=ads&fbclid=xyz", true, &buf)) {
        let pos: usize = base.len;
        * = appendVaryDimension(&buf, &pos, "Accept-Encoding", "  br, gzip ");
        const full_key = buf[0..pos];
        * = full_key; // "GET https://example.com/api?id=1\naccept-encoding:br, gzip"
    }
}
```
