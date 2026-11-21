# cap:uri_normalize

> 源码吸收自 `/data/dev/bun/src/url.zig`（RFC 3986 URI 规范化），提供纯 CP、零外部依赖、零堆分配的 URI 路径点段归约、百分号编码规范化与默认端口剥离库。

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
