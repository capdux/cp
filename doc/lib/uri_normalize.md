# cap:uri_normalize

> 参考来源：/data/dev/bun/src/url.zig | 吸收日期：2026-11-22

基于 RFC 3986 URI 规范化标准，提供纯 CP、零外部依赖、零堆分配的 URI 路径点段归约、百分号编码规范化与默认端口剥离库。

## 导入方式

```cp
import {
    isUnreserved,
    defaultPortForScheme,
    normalizePercentEncoding,
    removeDotSegments,
    normalizeOrigin,
} from "cap:uri_normalize";
```

## 核心 API

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `isUnreserved` | `fn isUnreserved(b: u8): bool` | RFC 3986 §2.3 非保留字符判定 |
| `defaultPortForScheme` | `fn defaultPortForScheme(scheme: []const u8): u16?` | 查询 `http`/`https`/`ws`/`wss`/`ftp`/`ssh`/`postgres`/`mysql`/`redis` 默认端口 |
| `normalizePercentEncoding` | `fn normalizePercentEncoding(input: []const u8, out: []u8): ([]const u8)?` | 解码非保留字符 `%XX`，其余合法 `%xx` 转大写，非法 `%` 返回 `null` |
| `removeDotSegments` | `fn removeDotSegments(path: []const u8, out: []u8): ([]const u8)?` | 严格遵循 RFC 3986 §5.2.4 归约 `.` 与 `..` |
| `normalizeOrigin` | `fn normalizeOrigin(scheme: []const u8, host: []const u8, port: u16?, out: []u8): ([]const u8)?` | 小写化 scheme/host 并自动剥离默认端口 |

## 使用示例

```cp
import { normalizePercentEncoding, removeDotSegments, normalizeOrigin } from "cap:uri_normalize";

fn example() {
    let buf: [128]u8 = undefined;
    * = normalizeOrigin("HTTPS", "Example.COM", 443, &buf);     // "https://example.com"
    * = removeDotSegments("/a/b/c/./../../g", &buf);            // "/a/g"
    * = normalizePercentEncoding("/%7euser/%2fdoc", &buf);      // "/~user/%2Fdoc"
}
```

## 上游行为契约与安全规范化边界

1. **保留字符不误解码 (`normalizePercentEncoding`)**：
   - 仅将 RFC 3986 §2.3 非保留字符（`ALPHA`/`DIGIT`/`-`/`.`/`_`/`~`，如 `%7e` -> `~`）解码为字面量；保留字符（如 `%2f` `/`、`%3f` `?`、`%23` `#`）仅统一转大写十六进制（`%2F`），绝不改变 URI 路径结构语义，防止路由鉴权绕过。
2. **点段归约与缓存键归一化 (`removeDotSegments` & `normalizeOrigin`)**：
   - 严格按 RFC 3986 §5.2.4 消解 `.` / `..` 路径段，配合 `normalizeOrigin` 小写化 scheme/host 并剥离默认端口（如 `https:443`、`http:80`），适合 CDN 缓存键去重与 SSRF 源校验。
